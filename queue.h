#include <stddef.h>

#define ERROR -1 /* Generic error when working with queue, check with result <= ERROR for less granular check */
#define ERROR_ALLOCATING_QUEUE -2
#define ERROR_QUEUE_IS_EMPTY -3
#define ERROR_ADDING_ELEMENT -4

typedef struct node
{
    struct node *next;
    struct node *prev;
    void *elem;
} node;

typedef struct
{
    node *head;
    size_t log_len;
    size_t elem_size;
    void (*dealloc)(void *);
} queue;

int queue_init(queue *q, size_t elem_size, void (*dispose)(void *));
int queue_dispose(queue *q);
void node_dispose(node *node, void (*dispose)(void *));
int queue_add(queue *q, void *elem);
int queue_poll(queue *q, void *elem);