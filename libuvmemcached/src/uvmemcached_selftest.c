#include "../include/uvmemcached.h"
#include "../include/private/pool.h"

int main(int argc, char *argv[])
{
    uv_loop_t* loop;
    char verbose;

    if (argc == 2 && strcmp(argv[1], "-v") == 0) {
        verbose = 1;
    } else {
        verbose = 0;
    }

    printf("Running UVMEMCACHED selftests...\n");

    loop = uv_default_loop();

    uv_memcached_conn_pool_test(loop, verbose);
    // uv_memcached_test(loop, verbose);

    uv_run(loop, UV_RUN_DEFAULT);

    assert(loop);
    uv_loop_delete(loop);

    printf("Tests passed OK\n");
    return 0;
}
