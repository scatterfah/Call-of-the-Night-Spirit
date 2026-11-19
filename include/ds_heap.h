#ifndef DS_HEAP_H
#define DS_HEAP_H

#include <stddef.h>

/* Comparison function: return a negative number if a should come out BEFORE b
   (a is "smaller"), zero if they are equal, a positive number otherwise.
   For auto-aim: compare distances to the player.
   For finishing blows: compare health. */
typedef int (*HeapCompare)(const void *a, const void *b);

/* A min-heap stored in a plain array of pointers.
   The heap does NOT own the data it stores: the caller creates and frees it. */
typedef struct {
    void **items;        /* array of pointers to the stored items */
    size_t size;         /* how many items are in the heap */
    size_t capacity;     /* how many slots the array has */
    HeapCompare compare; /* decides what "smaller" means */
} Heap;

Heap   *heap_create(HeapCompare compare);
int     heap_push(Heap *h, void *data);   /* returns 1 on success, 0 on failure */
void   *heap_pop(Heap *h);                /* removes and returns the smallest, NULL if empty */
void   *heap_peek(const Heap *h);         /* smallest item without removing it, NULL if empty */
size_t  heap_size(const Heap *h);
void    heap_clear(Heap *h);              /* removes all items (use before re-pushing each frame) */
void    heap_destroy(Heap *h);            /* frees the heap, not the stored data */

#endif