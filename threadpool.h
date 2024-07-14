#include <pthread.h>

struct thread_data {
    int socket;
    int (*func)(int socket);
};

struct tpconfig
{
    int max_pool_size;
};

struct tpool
{
    int curr_size;
    pthread_t *threads;
    struct tpconfig *config;
};

int tpexecute(struct tpool *tp, int socket, int (*func) (int socket));
void tpinit(struct tpool *tp, struct tpconfig *c);
void tpfree(struct tpool *tp);

void* run(void* thread_data);