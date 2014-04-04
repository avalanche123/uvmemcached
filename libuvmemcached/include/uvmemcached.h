#ifndef UV_MEMCACHED_H
#define UV_MEMCACHED_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>

typedef struct uv_memcached_conn_pool_s uv_memcached_conn_pool_t;
typedef struct uv_memcached_s uv_memcached_t;

typedef void (*uv_memcached_connect_cb)(uv_memcached_t* memcached, int status, void* context);
typedef void (*uv_memcached_disconnect_cb)(uv_memcached_t* memcached, void* context);
typedef void (*uv_memcached_set_cb)(uv_memcached_t* memcached, int status, void* context);
typedef void (*uv_memcached_get_cb)(uv_memcached_t* memcached, int status, char* data, void* context);

struct uv_memcached_s {
    uv_memcached_conn_pool_t*  pool;
    void*                      connect_context;
    uv_memcached_connect_cb    connect_callback;
    void*                      disconnect_context;
    uv_memcached_disconnect_cb disconnect_callback;
};

uv_memcached_t* uv_memcached_new(uv_loop_t* loop, unsigned int pool_size);
void uv_memcached_destroy(uv_memcached_t** self_p);

int uv_memcached_connect(uv_memcached_t* self, const char* connection, void* context, uv_memcached_connect_cb callback);
int uv_memcached_disconnect(uv_memcached_t* self, void* context, uv_memcached_disconnect_cb callback);

int uv_memcached_set(uv_memcached_t* self, const char* key, char* data, void* context, uv_memcached_set_cb callback);
int uv_memcached_get(uv_memcached_t* self, const char* key, void* context, uv_memcached_get_cb callback);

void uv_memcached_test(uv_loop_t* loop, char verbose);

#ifdef __cplusplus
}
#endif
#endif /* UV_MEMCACHED_H */
