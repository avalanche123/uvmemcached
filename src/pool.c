#include "../include/uvmemcached.h"
#include "../include/private/pool.h"

static uv_memcached_reserve_connection_callback_queue_t* uv_memcached_reserve_connection_callback_queue_new(unsigned int size);
static void uv_memcached_reserve_connection_callback_queue_destroy(uv_memcached_reserve_connection_callback_queue_t** self_p);
static int uv_memcached_reserve_connection_callback_queue_push(uv_memcached_reserve_connection_callback_queue_t* self, void* context, uv_memcached_conn_pool_reserve_connection_cb callback);
static uv_memcached_reserve_connection_callback_t* uv_memcached_reserve_connection_callback_queue_pop(uv_memcached_reserve_connection_callback_queue_t* self);

static void uv_memcached_conn_pool_on_connect(uv_connect_t* req, int status);
static void uv_memcached_conn_pool_on_close(uv_handle_t* handle);
static void uv_memcached_conn_pool_on_reserve_connection(uv_memcached_conn_t* connection, void* context);

uv_memcached_conn_pool_t*
uv_memcached_conn_pool_new(uv_loop_t* loop, unsigned int size)
{
    uv_memcached_conn_pool_t* pool;
    uv_tcp_t* tcp;
    uv_memcached_conn_t* connection;
    unsigned int i;

    if (size < 1) {
        return NULL;
    }

    pool = (uv_memcached_conn_pool_t*) calloc(1, sizeof(uv_memcached_conn_pool_t));

    assert(pool);

    pool->size        = size;
    pool->head        = 0;
    pool->tail        = 0;
    pool->available   = 0;
    pool->connected   = 0;
    pool->connections = (uv_tcp_t**) calloc(size, sizeof(uv_tcp_t*));

    assert(pool->connections);

    for (i = 0; i < pool->size; i++) {
        tcp = (uv_tcp_t*) calloc(1, sizeof(uv_tcp_t));

        assert(tcp);
        assert(uv_tcp_init(loop, tcp) == 0);

        connection = calloc(1, sizeof(uv_memcached_conn_t));

        assert(connection);

        connection->pool = pool;
        connection->tcp  = tcp;
        tcp->data        = connection;

        pool->connections[i] = tcp;
    }

    pool->callback_queue = uv_memcached_reserve_connection_callback_queue_new(255);

    return pool;
}

void
uv_memcached_conn_pool_destroy(uv_memcached_conn_pool_t** self_p)
{
    assert(self_p);

    unsigned int i;

    if (*self_p) {
        uv_memcached_conn_pool_t *self = *self_p;
        printf("Destroying connection pool %p\n", self);

        for (i = 0; i < self->size; i++) {
            free(self->connections[i]->data);
            free(self->connections[i]);
        }

        free(self->connections);
        uv_memcached_reserve_connection_callback_queue_destroy(&self->callback_queue);
        free(self);

        *self_p = NULL;
        printf("Destroyed connection pool\n");
    }
}

int
uv_memcached_conn_pool_connect(uv_memcached_conn_pool_t* self, const char* connection, void* context, uv_memcached_conn_pool_connect_cb callback)
{
    int i, port;
    char *str, *proto, *ip;

    str   = strdup(connection);
    proto = strtok(str, "://");
    ip    = strtok(NULL, "://");

    sscanf(strtok(NULL, ":"), "%d", &port);
    free(str);

    printf("protocol=%s ip=%s port=%d\n", proto, ip, port);

    if (strcmp("tcp", proto) != 0) {
        return -1;
    }

    self->on_connect_ctx = context;
    self->on_connect_cb  = callback;

    printf("Pool pointer is %p\n", self);

    struct sockaddr_in addr = uv_ip4_addr(ip, port);

    printf("Connect address initialized\n");

    for (i = 0; i < self->size; i++) {
        printf("In connection %d\n", i);
        uv_connect_t* req = (uv_connect_t*) calloc(1, sizeof(uv_connect_t));

        assert(req);

        req->data = self;

        printf("Current connection %d is %p\n", i, self->connections[i]);

        assert(self->connections[i]);

        uv_tcp_connect(req, self->connections[i], addr, uv_memcached_conn_pool_on_connect);
    }

    return 0;
}

int
uv_memcached_conn_pool_disconnect(uv_memcached_conn_pool_t* self, void* context, uv_memcached_conn_pool_disconnect_cb callback)
{
    int i;

    self->on_disconnect_ctx = context;
    self->on_disconnect_cb  = callback;

    for (i = 0; i < self->size; i++) {
        uv_memcached_conn_pool_reserve_connection(self, context, uv_memcached_conn_pool_on_reserve_connection);
    }

    return 0;
}

int
uv_memcached_conn_pool_reserve_connection(uv_memcached_conn_pool_t* self, void* context, uv_memcached_conn_pool_reserve_connection_cb callback)
{
    uv_tcp_t* tcp;
    unsigned int next;

    printf("in uv_memcached_conn_pool_reserve_connection, %d available\n", self->available);

    if (self->connected == 0) {
        return -1;
    }

    if (self->available > 0) {
        tcp = self->connections[self->head];
        assert(tcp);

        next = self->head + 1;

        if (next == self->size) {
            next = 0;
        }

        self->head = next;
        self->available--;

        printf("calling the reserve connection callback\n");

        callback((uv_memcached_conn_t*) tcp->data, context);

        return 0;
    } else {
        return uv_memcached_reserve_connection_callback_queue_push(self->callback_queue, context, callback);
    }

    return -1;
}

int uv_memcached_conn_pool_release_connection(uv_memcached_conn_t* connection)
{
    uv_memcached_conn_pool_t* pool;
    uv_memcached_reserve_connection_callback_t* callback;
    int result = -1;
    unsigned int next;

    pool = connection->pool;

    do {
        if (pool->available == pool->size) {
            break;
        }

        if (pool->callback_queue->count > 0) {
            callback = uv_memcached_reserve_connection_callback_queue_pop(pool->callback_queue);
            callback->callback(connection, callback->context);
            free(callback);
        }

        next = pool->tail + 1;

        if (next == pool->size) {
            next = 0;
        }

        pool->tail = next;
        pool->available++;
        result = 0;
    } while (0);

    return result;
}

/* BEGIN PRIVATE */

static uv_memcached_reserve_connection_callback_queue_t*
uv_memcached_reserve_connection_callback_queue_new(unsigned int size)
{
    uv_memcached_reserve_connection_callback_queue_t* queue;

    queue = (uv_memcached_reserve_connection_callback_queue_t*) calloc(1, sizeof(uv_memcached_reserve_connection_callback_queue_t));

    assert(queue);

    queue->size  = size;
    queue->count = 0;
    queue->head  = 0;
    queue->tail  = 0;

    queue->callbacks = (uv_memcached_reserve_connection_callback_t**) calloc(size, sizeof(uv_memcached_reserve_connection_callback_t*));

    assert(queue->callbacks);

    return queue;
}

static void
uv_memcached_reserve_connection_callback_queue_destroy(uv_memcached_reserve_connection_callback_queue_t** self_p)
{
    unsigned int i;
    assert(self_p);

    if (*self_p) {
        uv_memcached_reserve_connection_callback_queue_t *self = *self_p;
        printf("Destroying callback queue pool %p\n", self);

        for (i = 0; i < self->count; i++) {
            free(self->callbacks[i]);
        }

        free(self->callbacks);
        free(self);

        *self_p = NULL;
        printf("Destroyed callback queue pool\n");
    }
}

static int
uv_memcached_reserve_connection_callback_queue_push(uv_memcached_reserve_connection_callback_queue_t* self, void* context, uv_memcached_conn_pool_reserve_connection_cb callback)
{
    uv_memcached_reserve_connection_callback_t* cb;
    unsigned int next;
    int result = -1;

    do {
        if (self->count >= self->size) {
            break;
        }

        cb = calloc(1, sizeof(uv_memcached_reserve_connection_callback_t));

        if (cb == NULL) {
            break;
        }

        cb->callback = callback;
        cb->context  = context;

        next = self->tail + 1;
        if (next == self->size) {
            next = 0;
        }

        self->callbacks[self->tail] = cb;
        self->tail = next;
        self->count++;
    } while (0);

    return result;
}

static uv_memcached_reserve_connection_callback_t*
uv_memcached_reserve_connection_callback_queue_pop(uv_memcached_reserve_connection_callback_queue_t* self)
{
    uv_memcached_reserve_connection_callback_t* cb = NULL;
    unsigned int next;

    do {
        if (self->count == 0) {
            break;
        }

        cb   = self->callbacks[self->head];
        next = self->head + 1;

        if (next == self->size) {
            next = 0;
        }

        self->callbacks[self->head] = NULL;
        self->head = next;
        self->count--;
    } while (0);

    return cb;
}

static void
uv_memcached_conn_pool_on_connect(uv_connect_t* req, int status)
{
    uv_memcached_conn_pool_t* pool = (uv_memcached_conn_pool_t*) req->data;

    free(req);

    if (status == 0) {
        pool->available++;
        pool->tail++;
        pool->connected++;

        if (pool->connected == pool->size) {
            pool->on_connect_cb(pool, 0, pool->on_connect_ctx);
        }
    } else {
        pool->on_connect_cb(pool, -1, pool->on_connect_ctx);
    }
}

static void
uv_memcached_conn_pool_on_reserve_connection(uv_memcached_conn_t* connection, void* context)
{
    uv_close((uv_handle_t*) connection->tcp, uv_memcached_conn_pool_on_close);
}

static void
uv_memcached_conn_pool_on_close(uv_handle_t* handle)
{
    uv_tcp_t* tcp;
    uv_memcached_conn_t* connection;
    uv_memcached_conn_pool_t* pool;

    printf("handling connection close\n");

    tcp        = (uv_tcp_t*) handle;
    printf("getting connection\n");
    connection = (uv_memcached_conn_t*) tcp->data;
    printf("getting pool\n");
    pool       = connection->pool;

    printf("decrementing connected\n");
    pool->connected--;

    if (pool->connected == 0) {
        printf("running on disconnect\n");
        assert(pool->on_disconnect_cb);
        assert(pool->on_disconnect_ctx);
        pool->on_disconnect_cb(pool, pool->on_disconnect_ctx);
    }
}

/* END PRIVATE */

/* BEGIN TESTS */

static const char* ctx = "some context value";
static void on_connect(uv_memcached_conn_pool_t* pool, int status, void* context);
static void on_reserve_connection(uv_memcached_conn_t* connection, void* context);
static void on_write(uv_write_t* req, int status);
static void on_read(uv_stream_t* stream, ssize_t nread, uv_buf_t buf);
static uv_buf_t on_alloc(uv_handle_t* handle, size_t suggested_size);

void
uv_memcached_conn_pool_test(uv_loop_t* loop, char verbose)
{
    uv_memcached_conn_pool_t* pool;

    pool = uv_memcached_conn_pool_new(loop, 5);

    assert(uv_memcached_conn_pool_connect(pool, "tcp://127.0.0.1:11211", (void*) ctx, on_connect) == 0);
}

static void
on_connect(uv_memcached_conn_pool_t* pool, int status, void* context)
{
    assert(strcmp((char *) context, ctx) == 0);

    if (status != 0) {
        printf("Connection failed\n");
        assert(0);
    }

    assert(pool->available == pool->size);
    assert(uv_memcached_conn_pool_reserve_connection(pool, NULL, on_reserve_connection) == 0);
}

static void
on_reserve_connection(uv_memcached_conn_t* connection, void* context)
{
    assert(connection);

    char txt[250];

    char* key   = strdup("mykey");
    char* value = strdup("my value");

    uv_write_t* req;
    sprintf(txt, "set %s 0 0 %lu\r\n%s\r\n", key, strlen(value), value);

    uv_buf_t msg = uv_buf_init(txt, sizeof(txt));
    req = (uv_write_t*) calloc(1, sizeof(uv_write_t));

    assert(req);

    req->data = connection->pool;

    uv_write(req, (uv_stream_t*) connection->tcp, &msg, 1, on_write);
}

static void
on_write(uv_write_t* req, int status)
{
    assert(req);
    assert(status == 0);

    // int i;
    uv_tcp_t* connection;

    connection = (uv_tcp_t*) req->handle;

    uv_read_start((uv_stream_t*) connection, on_alloc, on_read);
}

static uv_buf_t
on_alloc(uv_handle_t* handle, size_t suggested_size)
{
    char* buffer = (char*) calloc(1, suggested_size);
    return uv_buf_init(buffer, sizeof(buffer));
}

static void
on_read(uv_stream_t* stream, ssize_t nread, uv_buf_t buf)
{
    uv_tcp_t* tcp = (uv_tcp_t*) stream;
    uv_memcached_conn_t* connection = (uv_memcached_conn_t*) tcp->data;
    printf("Read bytes %zd with %s\n", nread, buf.base);
    uv_read_stop(stream);
    uv_memcached_conn_pool_destroy(&connection->pool);
}

/* END TESTS */
