#include <pthread.h>

typedef struct
{
    int socket;
    int (*func)(int socket);
} thread_data;

typedef struct
{
    size_t max_pool_size;
} tpconfig;

typedef struct
{
    int curr_size;
    pthread_t *threads;
    tpconfig *config;
} tpool;

int tp_execute(int socket, int (*func)(int socket));
void tp_init(tpool *tp, tpconfig *c);
void tp_free(tpool *tp);
void *run(void *thread_id);