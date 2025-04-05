#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "threadpool.h"
#include "queue.h"
#include "semaphore.h"

static char *sem_common_lock_name = "/sem_common_lock";
static char *sem_queue_full_name = "/sem_queue_full";
static char *sem_queue_empty_name = "/sem_queue_empty";

static sem_t *sem_common_lock;
static sem_t *sem_queue_full;
static sem_t *sem_queue_empty;

static queue work_q;

static void thread_data_dispose(void *data)
{
	free(data);
}

static void unlink_semaphores()
{
	sem_unlink(sem_common_lock_name);
	sem_unlink(sem_queue_empty_name);
	sem_unlink(sem_queue_full_name);
}

void tp_init(tpool *tp, tpconfig *c)
{
	*(tp->config) = *c;
	tp->curr_size = 0;

	// allocate threads array
	pthread_t *t = malloc(sizeof(pthread_t) * c->max_pool_size);
	if (t == NULL)
	{
		exit(EXIT_FAILURE);
	}
	tp->threads = t;

	unlink_semaphores(); // in case an unexpected shutdown occurred before

	sem_queue_empty = sem_open(sem_queue_empty_name, O_CREAT, 0777, tp->config->max_pool_size); // full pool size available
	if (sem_queue_empty == SEM_FAILED)
	{
		fprintf(stderr, "%s\n", "ERROR creating semaphore sem_queue_empty");
		exit(EXIT_FAILURE);
	}

	sem_queue_full = sem_open(sem_queue_full_name, O_CREAT, 0777, 0); // queue initially empty
	if (sem_queue_full == SEM_FAILED)
	{
		fprintf(stderr, "%s\n", "ERROR creating semaphore sem_queue_full");
		exit(EXIT_FAILURE);
	}
	sem_common_lock = sem_open(sem_common_lock_name, O_CREAT, 0777, 1); // free lock
	if (sem_common_lock == SEM_FAILED)
	{
		fprintf(stderr, "%s\n", "ERROR creating semaphore common_lock");
		exit(EXIT_FAILURE);
	}

	// block inited threads until we release them
	sem_wait(sem_common_lock);

	while (tp->curr_size < c->max_pool_size)
	{
		int thread = pthread_create(tp->threads + tp->curr_size, NULL, run, (void *)(intptr_t)tp->curr_size);
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

int tp_execute(int socket, int (*func)(int))
{
	thread_data *data = malloc(sizeof(thread_data));
	data->socket = socket;
	data->func = func;

	sem_wait(sem_queue_empty); // decrease available slots
	sem_wait(sem_common_lock); // acquire mutex

	queue_add(&work_q, data);

	sem_post(sem_common_lock); // release mutex
	sem_post(sem_queue_full);  // notify consumers that the queue is not empty

	free(data);

	return 0;
}

// the worker thread function now checks for a shutdown signal (poison pill)
void *run(void *thread_id)
{
	thread_data *data = malloc(sizeof(thread_data));
	int poll = 0;
	while (1)
	{
		sem_wait(sem_queue_full);  // wait until something is in the queue
		sem_wait(sem_common_lock); // acquire mutex

		poll = queue_poll(&work_q, data);

		sem_post(sem_common_lock); // release mutex
		sem_post(sem_queue_empty); // notify producers that a slot is free

		if (!poll)
		{
			// check for poison pill: a NULL function pointer signals shutdown
			if (data->func == NULL)
			{
				break;
			}
			data->func(data->socket);
		}
	}

	free(data);
	return NULL;
}

// new function to gracefully shut down the thread pool.
void tp_shutdown(tpool *tp)
{
	int i;
	// enqueue a poison pill for each worker thread
	for (i = 0; i < tp->curr_size; i++)
	{
		thread_data *data = malloc(sizeof(thread_data));
		data->socket = 0;  // not used
		data->func = NULL; // poison pill signal

		sem_wait(sem_queue_empty);
		sem_wait(sem_common_lock);

		queue_add(&work_q, data);

		sem_post(sem_common_lock);
		sem_post(sem_queue_full);

		free(data);
	}
	// join all worker threads
	for (i = 0; i < tp->curr_size; i++)
	{
		pthread_join(tp->threads[i], NULL);
	}
}

// tp_free now calls tp_shutdown before cleaning up
void tp_free(tpool *tp)
{
	tp_shutdown(tp);

	sem_close(sem_common_lock);
	sem_unlink(sem_common_lock_name);

	sem_close(sem_queue_empty);
	sem_unlink(sem_queue_empty_name);

	sem_close(sem_queue_full);
	sem_unlink(sem_queue_full_name);

	queue_dispose(&work_q);
	free(tp->threads);
}
