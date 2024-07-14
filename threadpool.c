#include "threadpool.h"
#include <stdlib.h>
#include <stdio.h>

#define NO_AVAILABLE_THREAD -1

pthread_mutex_t common_lock = PTHREAD_MUTEX_INITIALIZER;

void tpinit(struct tpool *tp, struct tpconfig *c)
{
    *tp->config = *c;
    tp->curr_size = 0;

    printf("tpinit, max pool size %d, curr size is %d", c->max_pool_size, tp->curr_size);
    pthread_t t = malloc(sizeof(pthread_t) * c->max_pool_size);
    if (t == NULL)
    {
        exit(EXIT_FAILURE);
    }
    
    tp->threads = t;
}

// todo make more generic
int tpexecute(struct tpool *tp, int socket, int (*func)(int))
{
    
    int thread;
    printf("tpexecute, current size %d, max size is %d\n", tp->curr_size, (tp->config)->max_pool_size);
    if (tp->curr_size < (tp->config)->max_pool_size)
    {
        printf("create thread data\n");
        struct thread_data *data = malloc(sizeof(struct thread_data));
        data->socket = socket;
        data->func = func;
        
        printf("create thread\n");
        thread = pthread_create(tp->threads + tp->curr_size++, NULL, run, data);
        if (thread)
        {
            printf("ERROR: return code from pthread_create() is %d\n", thread);
            exit(-1);
        }

        return 0;
    }

    return NO_AVAILABLE_THREAD;
}

void* run(void *thread_data)
{    
    struct thread_data *data = thread_data;
    // critical section
    pthread_mutex_lock(&common_lock);
    printf("executing func\n");
    data->func(data->socket);
    pthread_mutex_unlock(&common_lock);
    
    free(thread_data);
    pthread_exit(NULL);
}

void tpfree(struct tpool *tp)
{
    free(tp->threads);
}