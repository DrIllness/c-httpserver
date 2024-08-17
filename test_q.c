#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

void dispose_int(void *elem)
{
    free(elem);
}

// clang -o q.out test_q.c queue.c && ./q.out
int main()
{
    queue q;
    queue_init(&q, sizeof(int), dispose_int);
    int ints[21];

    int res = 0;
    for (int i = 0; i < 21; i++)
    {
        ints[i] = i;
        queue_add(&q, ints + i);
    }

    while (queue_poll(&q, &res) != ERROR_QUEUE_IS_EMPTY)
    {
        printf("queue_poll: got %d from queue!\n", res);
    }

    printf("disposing...\n");
    queue_dispose(&q);
    printf("disposed\n");
}