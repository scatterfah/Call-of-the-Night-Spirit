#include <stdio.h>
#include <assert.h>
#include "ds_queue.h"

int main(void) {
    Queue *q = queue_create();
    assert(q != NULL);
    assert(queue_empty(q));
    assert(queue_size(q) == 0);
    assert(queue_dequeue(q) == NULL);      /* dequeue on empty must not crash */
    assert(queue_peek(q) == NULL);
    printf("queue_create OK\n");

    int a = 1, b = 2, c = 3;

    assert(queue_enqueue(q, &a));
    assert(queue_size(q) == 1);
    assert(q->head == q->tail);            /* one node: front and back */

    assert(queue_enqueue(q, &b));
    assert(queue_enqueue(q, &c));
    assert(queue_size(q) == 3);
    assert(*(int *)queue_peek(q) == 1);    /* peek shows the front */
    assert(queue_size(q) == 3);            /* peek removes nothing */
    printf("queue_enqueue OK\n");

    assert(*(int *)queue_dequeue(q) == 1); /* same order as they joined */
    assert(*(int *)queue_dequeue(q) == 2);
    assert(*(int *)queue_dequeue(q) == 3);
    assert(queue_empty(q));
    assert(q->head == NULL);
    assert(q->tail == NULL);               /* the classic bug: tail must be NULL too */
    assert(queue_dequeue(q) == NULL);
    printf("queue_dequeue OK\n");

    /* enqueue again after it became empty */
    assert(queue_enqueue(q, &b));
    assert(queue_size(q) == 1);
    assert(q->head == q->tail);
    assert(*(int *)queue_peek(q) == 2);
    printf("queue reuse OK\n");

    queue_destroy(q);
    printf("ALL QUEUE TESTS PASSED\n");
    return 0;
}