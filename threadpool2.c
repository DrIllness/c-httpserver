#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "threadpool.h"
#include "queue.h"
#include "semaphore.h"

char *sem_common_lock_name = "sem_common_lock";
char *sem_queue_full_name = "sem_queue_full";
char *sem_queue_empty_name = "sem_queue_empty";

sem_t *sem_common_lock;
sem_t *sem_queue_full;
sem_t *sem_queue_empty;

static queue work_q;

void thread_data_dispose(void *data)
{
    free(data);
}

void tp_init(tpool *tp, tpconfig *c)
{
	*(tp->config) = *c;
    tp->curr_size = 0;

    // init thread pool
    pthread_t *t = malloc(sizeof(pthread_t) * c->max_pool_size);
    if (t == NULL)
    {
        exit(EXIT_FAILURE);
    }
    tp->threads = t;

    sem_queue_empty = sem_open(sem_queue_empty_name, O_CREAT, 777, tp->config->max_pool_size); // full pool size available
    if (sem_queue_empty == SEM_FAILED)
    {
        fprintf(stderr, "%s\n", "ERROR creating semaphore sem_queue_empty");
        exit(EXIT_FAILURE);
    }

    sem_queue_full = sem_open(sem_queue_full_name, O_CREAT, 777, 0); // queue is not full on init
    if (sem_queue_full == SEM_FAILED)
    {
        fprintf(stderr, "%s\n", "ERROR creating semaphore sem_queue_full");
        exit(EXIT_FAILURE);
    }
    sem_common_lock = sem_open(sem_common_lock_name, O_CREAT, 777, 1); // free lock, so that we can proberen successfully
    if (sem_common_lock == SEM_FAILED)
    {
        fprintf(stderr, "%s\n", "ERROR creating semaphore common_lock");
        exit(EXIT_FAILURE);
    }

    sem_wait(sem_common_lock); // block so inited threads are not run

    while (tp->curr_size < c->max_pool_size)
    {
        int thread = pthread_create(tp->threads + tp->curr_size, NULL, run, (void *)tp->curr_size);
        tp->curr_size++;
        if (thread && tp->curr_size == 0) // exit only if no threads at all
        {
            printf("ERROR: return code from pthread_create() is %d\n", thread);
            exit(EXIT_FAILURE);
        }
    }

    queue_init(&work_q, sizeof(thread_data), thread_data_dispose);

    sem_post(sem_common_lock); // free so inited threads can run
}

int tp_execute(tpool *tp, int socket, int (*func)(int))
{
    thread_data *data = malloc(sizeof(thread_data));
    data->socket = socket;
    data->func = func;

    sem_wait(sem_queue_empty); // decrease available slots
    sem_wait(sem_common_lock); // acquire mutex

    queue_add(&work_q, data);

    sem_post(sem_common_lock); // release mutex
    sem_post(sem_queue_full);  // notify consumers, that its not full

    free(data);

    return 0;
}

void *run(void *thread_id)
{
    thread_data *data = malloc(sizeof(thread_data));
    int poll = 0;
    while (1)
    {
        sem_wait(sem_queue_full);  // wait till something in queue
        sem_wait(sem_common_lock); // acquire mutex

        poll = queue_poll(&work_q, data);

        sem_post(sem_common_lock); // releaase mutex
        sem_post(sem_queue_empty); // notify consumers, that its not empty

        if (!poll)
            data->func(data->socket);
    }

	printf("run: free(data)");
    free(data);
}

void tp_free(tpool *tp)
{
    sem_close(sem_common_lock);
    sem_unlink(sem_common_lock_name);

    sem_close(sem_queue_empty);
    sem_unlink(sem_queue_empty_name);

    sem_close(sem_queue_full);
    sem_unlink(sem_queue_full_name);

    queue_dispose(&work_q);
    free(tp->threads);
}