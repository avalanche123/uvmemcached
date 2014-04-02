#ifndef UV_MEMCACHED_POOL_H
#define UV_MEMCACHED_POOL_H

typedef struct uv_memcached_conn_s uv_memcached_conn_t;

typedef void (*uv_memcached_conn_pool_connect_cb)(uv_memcached_conn_pool_t* pool, int status, void* context);
typedef void (*uv_memcached_conn_pool_disconnect_cb)(uv_memcached_conn_pool_t* pool, void* context);
typedef void (*uv_memcached_conn_pool_reserve_connection_cb)(uv_memcached_conn_t* connection, void* context);

struct uv_memcached_conn_s {
    uv_memcached_conn_pool_t*  pool;
    uv_tcp_t*                  tcp;
};

typedef struct {
    void*                                    context;
    uv_memcached_conn_pool_reserve_connection_cb callback;
} uv_memcached_reserve_connection_callback_t;

typedef struct {
    unsigned int                             size;
    unsigned int                             count;
    unsigned int                             head;
    unsigned int                             tail;
    uv_memcached_reserve_connection_callback_t** callbacks;
} uv_memcached_reserve_connection_callback_queue_t;

struct uv_memcached_conn_pool_s {
    unsigned int                                      connecting;
    unsigned int                                      connected;
    unsigned int                                      closed;
    unsigned int                                      size;
    unsigned int                                      head;
    unsigned int                                      tail;
    unsigned int                                      available;
    uv_tcp_t**                                        connections;
    uv_memcached_conn_pool_connect_cb                 on_connect_cb;
    void*                                             on_connect_ctx;
    uv_memcached_conn_pool_disconnect_cb              on_close_cb;
    void*                                             on_close_ctx;
    uv_memcached_reserve_connection_callback_queue_t* callback_queue;
};

uv_memcached_conn_pool_t* uv_memcached_conn_pool_new(uv_loop_t* loop, unsigned int size);
void uv_memcached_conn_pool_destroy(uv_memcached_conn_pool_t** self_p);

int uv_memcached_conn_pool_connect(uv_memcached_conn_pool_t* self, const char* connection, void* context, uv_memcached_conn_pool_connect_cb callback);
int uv_memcached_conn_pool_disconnect(uv_memcached_conn_pool_t* self, void* context, uv_memcached_conn_pool_disconnect_cb callback);

int uv_memcached_conn_pool_reserve_connection(uv_memcached_conn_pool_t* self, void* context, uv_memcached_conn_pool_reserve_connection_cb callback);
int uv_memcached_conn_pool_release_connection(uv_memcached_conn_t* self);

void uv_memcached_conn_pool_test(uv_loop_t* loop, char verbose);

#endif /* UV_MEMCACHED_POOL_H */
