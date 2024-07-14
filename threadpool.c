#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "threadpool.h"
#include "queue.h"

#define NO_AVAILABLE_THREAD -1
#define IS_DEBUG 0

pthread_mutex_t common_lock = PTHREAD_MUTEX_INITIALIZER;

static queue work_q;

static int thread_called[5];

void thread_data_dispose(void *data)
{
    free(data);
}

void *count_down_to_exit(void *)
{
    struct timespec ts;
    ts.tv_sec = 10;
    ts.tv_nsec = 0;
    nanosleep(&ts, NULL);

    int i = 0;
    while (i < 5)
    {
        printf("Thread #%d callsed %d times\n", i, thread_called[i]);
        i++;
    }
    exit(EXIT_SUCCESS);
}

void test_pthread_spread()
{
    void *t = malloc(sizeof(pthread_t));
    pthread_create(t, NULL, count_down_to_exit, NULL);
}

void tp_init(tpool *tp, tpconfig *c)
{
    *tp->config = *c;
    tp->curr_size = 0;

    // init thread pool
    pthread_t t = malloc(sizeof(pthread_t) * c->max_pool_size);
    if (t == NULL)
    {
        exit(EXIT_FAILURE);
    }
    tp->threads = t;
    printf("allocated threads\n");

    pthread_mutex_lock(&common_lock);
    while (tp->curr_size < c->max_pool_size)
    {
        printf("init thread #%d\n", (tp->curr_size));
        int thread = pthread_create(tp->threads + tp->curr_size, NULL, run, (void *)tp->curr_size);
        tp->curr_size++;
        printf("passed pthread_create\n");
        if (thread && tp->curr_size == 0) // exit only if no threads at all
        {
            printf("ERROR: return code from pthread_create() is %d\n", thread);
            exit(-1);
        }
    }

    pthread_mutex_unlock(&common_lock);

    // init working queue
    printf("init working queue\n");
    queue_init(&work_q, sizeof(thread_data), thread_data_dispose);

    if (IS_DEBUG)
        test_pthread_spread();
}

int tp_execute(tpool *tp, int socket, int (*func)(int))
{
    printf("create thread data\n");

    thread_data *data = malloc(sizeof(thread_data));
    data->socket = socket;
    data->func = func;

    // post job
    printf("posting job...\n");
    pthread_mutex_lock(&common_lock);
    queue_add(&work_q, data);
    pthread_mutex_unlock(&common_lock);

    free(data);

    return 0;
}

void *run(void *thread_id)
{
    thread_data *data = malloc(sizeof(thread_data));
    int poll = 0;
    // infinite polling
    while (1)
    {
        pthread_mutex_lock(&common_lock);
        thread_called[(int)thread_id]++;
        poll = queue_poll(&work_q, data);

        pthread_mutex_unlock(&common_lock);

        if (!poll)
        {
            printf("served client in %d thread\n", (int)thread_id);
            data->func(data->socket);
        }
            
    }

    free(data);
}

void tp_free(tpool *tp)
{
    queue_dispose(&work_q);
    free(tp->threads);
}