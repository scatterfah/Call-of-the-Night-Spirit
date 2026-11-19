#ifndef DS_QUEUE_H
#define DS_QUEUE_H

#include <stddef.h>
#include <stdbool.h>

/* One node = one waiting item (for example one zombie in a wave). */
typedef struct QNode {
    void *data;              /* points to the item stored here */
    struct QNode *next;      /* the node behind this one in the line */
} QNode;

/* The queue itself. */
typedef struct {
    QNode *head;             /* front of the line: next to leave */
    QNode *tail;             /* back of the line: last to join */
    size_t size;             /* how many are waiting */
} Queue;

Queue *queue_create(void);
void   queue_destroy(Queue *q);
bool   queue_enqueue(Queue *q, void *data);   /* add at the back */
void  *queue_dequeue(Queue *q);               /* take from the front, NULL if empty */
void  *queue_peek(const Queue *q);            /* look at the front, NULL if empty */
bool   queue_empty(const Queue *q);
size_t queue_size(const Queue *q);

#endif