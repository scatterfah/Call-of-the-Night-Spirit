#include <stdlib.h>
#include <string.h>
#include "ds_pool.h"

Pool *pool_create(size_t item_size, size_t capacity){
    if(item_size == 0 || capacity == 0){
        return NULL;
    }
    Pool *p = malloc(sizeof(Pool));
    if(p == NULL){
        return NULL;
    }
    p->slots = malloc(item_size * capacity);
    p->active = calloc(capacity, sizeof(unsigned char));   /* all 0 = all free */
    p->free_slots = stack_create();
    if(p->slots == NULL || p->active == NULL || p->free_slots == NULL){
        free(p->slots);
        free(p->active);
        stack_destroy(p->free_slots);
        free(p);
        return NULL;
    }
    p->item_size = item_size;
    p->capacity = capacity;
    p->active_count = 0;

    /* every slot starts free. Push in reverse so slot 0 is acquired first. */
    for(size_t i = capacity; i > 0; i--){
        if(!stack_push(p->free_slots, p->slots + (i - 1) * item_size)){
            free(p->slots);
            free(p->active);
            stack_destroy(p->free_slots);
            free(p);
            return NULL;
        }
    }
    return p;
}

void *pool_acquire(Pool *p){
    if(p == NULL){
        return NULL;
    }
    void *slot = stack_pop(p->free_slots);
    if(slot == NULL){
        return NULL;                          /* pool is full */
    }
    size_t index = ((char *)slot - p->slots) / p->item_size;
    p->active[index] = 1;
    p->active_count++;
    memset(slot, 0, p->item_size);            /* a reused slot must not keep old data */
    return slot;
}

int pool_release(Pool *p, void *item){
    if(p == NULL || item == NULL){
        return 0;
    }
    char *addr = (char *)item;
    if(addr < p->slots || addr >= p->slots + p->item_size * p->capacity){
        return 0;                             /* not from this pool */
    }
    size_t offset = (size_t)(addr - p->slots);
    if(offset % p->item_size != 0){
        return 0;                             /* not the start of a slot */
    }
    size_t index = offset / p->item_size;
    if(!p->active[index]){
        return 0;                             /* already free: ignore double release */
    }
    p->active[index] = 0;
    p->active_count--;
    stack_push(p->free_slots, item);          /* never grows: at most capacity items */
    return 1;
}

size_t pool_active_count(const Pool *p){
    if(p == NULL){
        return 0;
    }
    return p->active_count;
}

size_t pool_capacity(const Pool *p){
    if(p == NULL){
        return 0;
    }
    return p->capacity;
}

void *pool_get(const Pool *p, size_t index){
    if(p == NULL || index >= p->capacity || !p->active[index]){
        return NULL;
    }
    return p->slots + index * p->item_size;
}

void pool_destroy(Pool *p){
    if(p == NULL){
        return;
    }
    stack_destroy(p->free_slots);
    free(p->active);
    free(p->slots);
    free(p);
}