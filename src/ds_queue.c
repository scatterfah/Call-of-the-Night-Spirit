#include <stdlib.h>
#include "ds_queue.h"

Queue *queue_create(void) {
    Queue *q = malloc(sizeof(Queue));
    if (q == NULL) {
        return NULL;
    }
    q->head = NULL;          /* empty line: nobody at the front */
    q->tail = NULL;          /* empty line: nobody at the back */
    q->size = 0;
    return q;
}

bool queue_enqueue(Queue *q, void *data) {
    if (q == NULL) return false;

    QNode *node = malloc(sizeof(QNode));
    if (node == NULL) return false;

    node->data = data;
    node->next = NULL;       /* it joins at the back, so nobody is behind it */

    if (q->tail == NULL) {
        q->head = node;          /* empty queue: it is also the front */
    } else {
        q->tail->next = node;    /* the old last one now points to it */
    }

    q->tail = node;              /* it is the back of the line now */
    q->size++;
    return true;
}

void *queue_dequeue(Queue *q) {
    if (q == NULL || q->head == NULL) {
        return NULL;             /* nothing to take */
    }

    QNode *node = q->head;       /* the one at the front */
    void *data = node->data;

    q->head = node->next;        /* the next one becomes the front */
    if (q->head == NULL) {
        q->tail = NULL;          /* we took the last one: the queue is empty */
    }

    free(node);
    q->size--;
    return data;
}

void *queue_peek(const Queue *q) {
    if (q == NULL || q->head == NULL) {
        return NULL;
    }
    return q->head->data;
}

bool queue_empty(const Queue *q) {
    return q == NULL || q->size == 0;
}

size_t queue_size(const Queue *q) {
    if (q == NULL) return 0;
    return q->size;
}

/* Frees the nodes and the queue. It does NOT free the data the nodes point to. */
void queue_destroy(Queue *q) {
    if (q == NULL) return;

    QNode *node = q->head;
    while (node != NULL) {
        QNode *next = node->next;    /* save next BEFORE freeing the node */
        free(node);
        node = next;
    }
    free(q);
}