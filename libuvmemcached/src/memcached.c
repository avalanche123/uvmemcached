#include "../include/uvmemcached.h"
#include "../include/private/pool.h"

typedef struct {
    uv_memcached_t*      client;
    void*                context;
    uv_memcached_set_cb  callback;
    uv_memcached_conn_t* connection;
    uv_tcp_t*            tcp;
    const char*          key;
    char*                data;
    uv_buf_t*            msg;
} uv_memcached_set_req_t;

typedef struct {
    uv_memcached_t*      client;
    void*                context;
    uv_memcached_get_cb  callback;
    uv_memcached_conn_t* connection;
    uv_tcp_t*            tcp;
    const char*          key;
    uv_buf_t*            msg;
} uv_memcached_get_req_t;

uv_memcached_t*
uv_memcached_new(uv_loop_t* loop, unsigned int pool_size)
{
    uv_memcached_t* self = (uv_memcached_t*) calloc(1, sizeof(uv_memcached_t));

    if (self) {
        self->pool = uv_memcached_conn_pool_new(loop, pool_size);

        assert(self->pool);
    }

    return self;
}

void
uv_memcached_destroy(uv_memcached_t** self_p)
{
    assert(self_p);

    if (*self_p) {
        uv_memcached_t *self = *self_p;

        uv_memcached_conn_pool_destroy(&self->pool);
        free(self);

        *self_p = NULL;
    }
}

static void uv_memcached_on_connect(uv_memcached_conn_pool_t* pool, int status, void* context);
static void uv_memcached_on_disconnect(uv_memcached_conn_pool_t* pool, void* context);
static void uv_memcached_set_on_reserve_connection(uv_memcached_conn_t* connection, void* context);
static void uv_memcached_set_on_write(uv_write_t* req, int status);
static void uv_memcached_set_on_read(uv_stream_t* stream, ssize_t nread, uv_buf_t buf);
static void uv_memcached_get_on_reserve_connection(uv_memcached_conn_t* connection, void* context);
static void uv_memcached_get_on_write(uv_write_t* req, int status);
static void uv_memcached_get_on_read(uv_stream_t* stream, ssize_t nread, uv_buf_t buf);
static uv_buf_t uv_memcached_on_alloc(uv_handle_t* handle, size_t suggested_size);

int
uv_memcached_connect(uv_memcached_t* self, const char* connection, void* context, uv_memcached_connect_cb callback)
{
    self->connect_context  = context;
    self->connect_callback = callback;

    return uv_memcached_conn_pool_connect(self->pool, connection, self, uv_memcached_on_connect);
}

int
uv_memcached_disconnect(uv_memcached_t* self, void* context, uv_memcached_disconnect_cb callback)
{
    self->disconnect_context  = context;
    self->disconnect_callback = callback;

    return uv_memcached_conn_pool_disconnect(self->pool, self, uv_memcached_on_disconnect);
}

int
uv_memcached_set(uv_memcached_t* self, const char* key, char* data, void* context, uv_memcached_set_cb callback)
{
    uv_write_t* req;
    uv_memcached_set_req_t* info;

    info = (uv_memcached_set_req_t*) calloc(1, sizeof(uv_memcached_set_req_t));
    assert(info);

    req = (uv_write_t*) calloc(1, sizeof(uv_write_t));
    assert(req);

    info->client     = self;
    info->context    = context;
    info->callback   = callback;
    info->connection = NULL;
    info->tcp        = NULL;
    info->msg        = NULL;
    info->key        = key;
    info->data       = data;
    req->data        = info;

    return uv_memcached_conn_pool_reserve_connection(self->pool, req, uv_memcached_set_on_reserve_connection);
}

int
uv_memcached_get(uv_memcached_t* self, const char* key, void* context, uv_memcached_get_cb callback)
{
    uv_write_t* req;
    uv_memcached_get_req_t* info;

    info = (uv_memcached_get_req_t*) calloc(1, sizeof(uv_memcached_get_req_t));
    assert(info);

    req = (uv_write_t*) calloc(1, sizeof(uv_write_t));
    assert(req);

    info->client     = self;
    info->context    = context;
    info->callback   = callback;
    info->connection = NULL;
    info->tcp        = NULL;
    info->msg        = NULL;
    info->key        = key;
    req->data        = info;

    return uv_memcached_conn_pool_reserve_connection(self->pool, req, uv_memcached_get_on_reserve_connection);
}

/* BEGIN PRIVATE */

static void
uv_memcached_on_connect(uv_memcached_conn_pool_t* pool, int status, void* context)
{
    uv_memcached_connect_cb callback;
    uv_memcached_t* self;

    self     = (uv_memcached_t*) context;
    callback = self->connect_callback;
    context  = self->connect_context;

    self->connect_callback = NULL;
    self->connect_context  = NULL;

    callback(self, status, context);
}

static void
uv_memcached_on_disconnect(uv_memcached_conn_pool_t* pool, void* context)
{
    uv_memcached_disconnect_cb callback;
    uv_memcached_t* self;

    self     = (uv_memcached_t*) context;
    callback = self->disconnect_callback;
    context  = self->disconnect_context;

    self->disconnect_callback = NULL;
    self->disconnect_context  = NULL;

    callback(self, context);
}

static void
uv_memcached_set_on_reserve_connection(uv_memcached_conn_t* connection, void* context)
{
    uv_buf_t* msg;
    uv_write_t* req;
    uv_memcached_set_req_t* info;
    char len[16];

    assert(context);

    req  = (uv_write_t*) context;
    info = (uv_memcached_set_req_t*) req->data;

    sprintf(len, "%zu", strlen(info->data));

    msg = calloc(7, sizeof(uv_buf_t));
    assert(msg);

    msg[0].base = "set ";
    msg[0].len  = 4;
    msg[1].base = (char*) strdup(info->key);
    msg[1].len  = strlen(info->key);
    msg[2].base = " 0 0 ";
    msg[2].len  = 5;
    msg[3].base = (char*) strdup(len);
    msg[3].len  = strlen(len);
    msg[4].base = "\r\n";
    msg[4].len  = 2;
    msg[5].base = (char*) strdup(info->data);
    msg[5].len  = strlen(info->data);
    msg[6].base = "\r\n";
    msg[6].len  = 2;

    info->msg = msg;

    uv_write(req, (uv_stream_t*) connection->tcp, msg, 7, uv_memcached_set_on_write);
}

static void
uv_memcached_set_on_write(uv_write_t* req, int status)
{
    uv_memcached_set_req_t* info;
    uv_tcp_t* tcp;

    info = (uv_memcached_set_req_t*) req->data;
    free(info->msg[1].base);
    free(info->msg[3].base);
    free(info->msg[5].base);
    free(info->msg);

    if (status == 0) {
        tcp  = (uv_tcp_t*) req->handle;

        info->connection = (uv_memcached_conn_t*) tcp->data;
        info->tcp        = tcp;
        tcp->data        = info;

        uv_read_start((uv_stream_t*) tcp, uv_memcached_on_alloc, uv_memcached_set_on_read);
    } else {
        // TODO: replace pool connection
        info->callback(info->client, status, info->context);
        free(info);
    }

    free(req);
}

static void
uv_memcached_set_on_read(uv_stream_t* stream, ssize_t nread, uv_buf_t buf)
{
    uv_memcached_set_req_t* info;

    uv_read_stop(stream);

    info = (uv_memcached_set_req_t*) stream->data;
    stream->data = info->connection;

    uv_memcached_conn_pool_release_connection(info->connection);

    // TODO: handle different errors
    if (strcmp(buf.base, "STORED\r\n") == 0) { // success
        info->callback(info->client, 0, info->context);
    } else {
        info->callback(info->client, -1, info->context);
    }
    
    free(buf.base);
    free(info);
}

static void
uv_memcached_get_on_reserve_connection(uv_memcached_conn_t* connection, void* context)
{
    uv_buf_t* msg;
    uv_write_t* req;
    uv_memcached_get_req_t* info;

    assert(context);

    req  = (uv_write_t*) context;
    info = (uv_memcached_get_req_t*) req->data;

    msg = calloc(3, sizeof(uv_buf_t));
    assert(msg);

    msg[0].base = "get ";
    msg[0].len  = 4;
    msg[1].base = (char*) strdup(info->key);
    msg[1].len  = strlen(info->key);
    msg[2].base = "\r\n";
    msg[2].len  = 2;

    info->msg = msg;

    uv_write(req, (uv_stream_t*) connection->tcp, msg, 3, uv_memcached_get_on_write);
}

static void
uv_memcached_get_on_write(uv_write_t* req, int status)
{
    uv_memcached_get_req_t* info;
    uv_tcp_t* tcp;

    info = (uv_memcached_get_req_t*) req->data;

    free(info->msg[1].base);
    free(info->msg);

    if (status == 0) {
        tcp  = (uv_tcp_t*) req->handle;

        info->connection = (uv_memcached_conn_t*) tcp->data;
        info->tcp        = tcp;
        tcp->data        = info;

        uv_read_start((uv_stream_t*) tcp, uv_memcached_on_alloc, uv_memcached_get_on_read);
    } else {
        info->callback(info->client, status, NULL, info->context);
        free(info);
    }

    free(req);
}

static void
uv_memcached_get_on_read(uv_stream_t* stream, ssize_t nread, uv_buf_t buf)
{
    uv_memcached_get_req_t* info;

    uv_read_stop(stream);

    info = (uv_memcached_get_req_t*) stream->data;
    stream->data = info->connection;

    uv_memcached_conn_pool_release_connection(info->connection);

    size_t len = 0;
    char   slen[16];
    char   key[255];

    sscanf(buf.base, "VALUE %s 0 %zu\r\n", key, &len);
    sprintf(slen, "%zu", len);

    char* data = calloc(len + 1, sizeof(char));
    assert(data);
    strncpy(data, buf.base + 6 + strlen(key) + 3 + strlen(slen) + 2, len);
    data[len] = '\0';

    if (len > 0) { // success
        info->callback(info->client, 0, data, info->context);
    } else {
        info->callback(info->client, -1, NULL, info->context);
    }

    free(buf.base);
    free(info);
}

static uv_buf_t
uv_memcached_on_alloc(uv_handle_t* handle, size_t suggested_size)
{
    uv_buf_t buf;
    buf.base = (char*) calloc(1, suggested_size);
    assert(buf.base);
    buf.len = suggested_size;

    return buf;
}

/* END PRIVATE */

/* BEGIN TESTS */

static void on_set(uv_memcached_t* memcached, int status, void* context);
static void on_get(uv_memcached_t* memcached, int status, char* data, void* context);
static void on_connect(uv_memcached_t* memcached, int status, void* context);
static void on_disconnect(uv_memcached_t* memcached, void* context);

static void
on_set(uv_memcached_t* memcached, int status, void* context)
{
    assert(memcached);
    assert(context);

    assert(uv_memcached_get(memcached, "somekey", context, on_get) == 0);
}

static void
on_get(uv_memcached_t* memcached, int status, char* data, void* context)
{
    assert(memcached);
    assert(context);

    assert(strcmp(data, (char*) context) == 0);

    free(data);
    free(context);

    data    = NULL;
    context = NULL;

    assert(uv_memcached_disconnect(memcached, NULL, on_disconnect) == 0);
}

static void
on_connect(uv_memcached_t* memcached, int status, void* context)
{
    char* data = (char*) strdup("this is my key value");

    assert(memcached);
    assert(context == NULL);

    if (status == 0) {
        assert(uv_memcached_set(memcached, "somekey", data, (void*) data, on_set) == 0);
    } else {
        free(data);
        uv_memcached_destroy(&memcached);
    }
}

static void
on_disconnect(uv_memcached_t* memcached, void* context)
{
    assert(memcached);
    assert(context == NULL);

    uv_memcached_destroy(&memcached);
}

void
uv_memcached_test(uv_loop_t* loop, char verbose)
{
    uv_memcached_t* client;
    
    client = uv_memcached_new(loop, 1);

    assert(client);
    
    assert(uv_memcached_connect(client, "tcp://127.0.0.1:11211", NULL, on_connect) == 0);
}

/* END TESTS */
