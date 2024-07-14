#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

int queue_init(queue *q, size_t elem_size, void (*dispose)(void *))
{
    q->elem_size = elem_size;
    q->log_len = 0;
    q->head = NULL;
    q->dealloc = dispose;
    return 0;
}

int queue_dispose(queue *q)
{
    if (q->head == NULL)
    {
        return ERROR_QUEUE_IS_EMPTY;
    }
    node_dispose(q->head, q->dealloc);

    return 0;
}

void node_dispose(node *node, void (*dispose)(void *))
{
    while ((node->prev) != NULL)
    {
        node_dispose(node->prev, dispose);
    }

    dispose(node->elem);
    free(node);
}

int queue_add(queue *q, void *elem)
{
    node *new_node = malloc(sizeof(node));
    new_node->elem = malloc(q->elem_size);
    new_node->prev = NULL;
    new_node->next = NULL;

    memcpy(new_node->elem, elem, q->elem_size);

    if (q->head == NULL)
    {
        q->head = new_node;
        return 0;
    }

    node *node = q->head;
    while (node->prev != NULL)
        node = node->prev;

    node->prev = new_node;
    new_node->next = node;

    q->log_len++;
}

int queue_poll(queue *q, void *elem)
{
    if (q->head == NULL)
        return ERROR_QUEUE_IS_EMPTY;

    memcpy(elem, q->head->elem, q->elem_size);

    node *new_head = q->head->prev;

    if (new_head != NULL)
        new_head->next = NULL;

    // clear old head and elem
    q->dealloc(q->head->elem);
    free(q->head);

    q->head = new_head;
    q->log_len--;

    return 0;
}