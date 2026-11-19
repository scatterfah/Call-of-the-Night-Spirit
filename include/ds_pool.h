#ifndef DS_POOL_H
#define DS_POOL_H

#include <stddef.h>
#include "ds_stack.h"

/* An object pool: all slots are allocated once in pool_create, and nothing is
   allocated while the game runs. Each slot is either free or active.
   Free slots are kept in a Stack, so acquire and release are both O(1). */
typedef struct {
    char *slots;            /* one block of memory holding every slot */
    unsigned char *active;  /* active[i] is 1 if slot i is in use, 0 if free */
    size_t item_size;       /* size of one slot in bytes */
    size_t capacity;        /* total number of slots */
    size_t active_count;    /* how many slots are in use right now */
    Stack *free_slots;      /* stack of pointers to the free slots */
} Pool;

Pool   *pool_create(size_t item_size, size_t capacity);
void   *pool_acquire(Pool *p);                 /* a zeroed free slot, or NULL if the pool is full */
int     pool_release(Pool *p, void *item);     /* 1 if released, 0 if invalid or already free */
size_t  pool_active_count(const Pool *p);
size_t  pool_capacity(const Pool *p);
void   *pool_get(const Pool *p, size_t index); /* slot i if it is active, otherwise NULL */
void    pool_destroy(Pool *p);

#endif