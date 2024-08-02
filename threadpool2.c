#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "threadpool.h"
#include "queue.h"
#include "semaphore.h"

#define NO_AVAILABLE_THREAD -1

char *sem_common_lock_name = "sem_common_lock3";
char *sem_queue_full_name = "sem_queue_full3";
char *sem_queue_empty_name = "sem_queue_empty3";

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

    printf("ini semaphores\n");
    sem_queue_empty = sem_open(sem_queue_empty_name, O_CREAT, 777, tp->config->max_pool_size); // full pool size available
    if (sem_queue_empty == SEM_FAILED)
    {
        fprintf(stderr, "%s\n", "ERROR creating semaphore sem_queue_empty");
        exit(EXIT_FAILURE);
    }
    printf("sem_queue_empty descriptor == %d\n", sem_queue_empty);

    sem_queue_full = sem_open(sem_queue_full_name, O_CREAT, 777, 0); // queue is not full on init
    if (sem_queue_full == SEM_FAILED)
    {
        fprintf(stderr, "%s\n", "ERROR creating semaphore sem_queue_full");
        exit(EXIT_FAILURE);
    }
    printf("sem_queue_full descriptor == %d\n", sem_queue_full);

    sem_common_lock = sem_open(sem_common_lock_name, O_CREAT, 777, 1); // free lock, so that we can proberen successfully
    if (sem_common_lock == SEM_FAILED)
    {
        fprintf(stderr, "%s\n", "ERROR creating semaphore common_lock");
        exit(EXIT_FAILURE);
    }
    printf("sem_common_lock descriptor == %d\n", sem_common_lock);

    printf("acquiring sem_common_lock\n");
    sem_wait(sem_common_lock); // block so inited threads are not run

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

    // init working queue
    printf("initing working queue...\n");
    queue_init(&work_q, sizeof(thread_data), thread_data_dispose);
    printf("init working queue DONE\n");

    sem_post(sem_common_lock); // free so inited threads are not run
    printf("tp_init finished\n");
}

int tp_execute(tpool *tp, int socket, int (*func)(int))
{
    printf("create thread data\n");

    thread_data *data = malloc(sizeof(thread_data));
    data->socket = socket;
    data->func = func;

    // post job
    printf("posting job...\n");
    printf("sem_queue_empty in tp_execute: acquiring...\n");
    sem_wait(sem_queue_empty); // decrease available slots
    printf("sem_queue_empty in tp_execute: acquired\n");
    printf("sem_common_lock in tp_execute: acquiring...\n");
    sem_wait(sem_common_lock); // acquire mutex
    printf("sem_common_lock in tp_execute: acquired\n");

    queue_add(&work_q, data);
    printf("sem_common_lock in tp_execute: releasing...\n");
    sem_post(sem_common_lock); // release mutex
    printf("sem_common_lock in tp_execute: released\n");
    printf("sem_queue_full in tp_execute: releasing...\n");
    sem_post(sem_queue_full); // notify consumers, that its not full
    printf("sem_queue_full in tp_execute: released\n");

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
        printf("sem_queue_full in run: acquiring...\n");
        sem_wait(sem_queue_full);
        printf("sem_queue_full in run: acquired\n");
        printf("sem_common_lock in run: acquiring...\n");
        sem_wait(sem_common_lock); // acquire
        printf("sem_common_lock in run: acquired\n");

        poll = queue_poll(&work_q, data);

        printf("sem_common_lock in run: releasing...\n");
        sem_post(sem_common_lock); // releaase
        printf("sem_common_lock in run: released\n");
        printf("sem_queue_empty in run: releasing...\n");
        sem_post(sem_queue_empty);
        printf("sem_queue_empty in run: released\n");

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
    // close semaphore
    sem_close(sem_common_lock);
    sem_unlink(sem_common_lock_name);

    sem_close(sem_queue_empty);
    sem_unlink(sem_queue_empty_name);

    sem_close(sem_queue_full);
    sem_unlink(sem_queue_full_name);

    queue_dispose(&work_q);
    free(tp->threads);
}