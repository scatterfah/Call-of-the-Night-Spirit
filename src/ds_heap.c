#include <stdlib.h>
#include "ds_heap.h"

#define HEAP_START_CAPACITY 8

/* Index maths for an array-based tree. */
#define PARENT(i) (((i) - 1) / 2)
#define LEFT(i)   (2 * (i) + 1)
#define RIGHT(i)  (2 * (i) + 2)

static void swap(void **a, void **b){
    void *tmp = *a;
    *a = *b;
    *b = tmp;
}

/* Move the item at index i UP until its parent is not larger than it. */
static void sift_up(Heap *h, size_t i){
    while(i > 0){
        size_t parent = PARENT(i);
        if(h->compare(h->items[i], h->items[parent]) < 0){
            swap(&h->items[i], &h->items[parent]);
            i = parent;
        } else {
            break;
        }
    }
}

/* Move the item at index i DOWN until it is not larger than both children. */
static void sift_down(Heap *h, size_t i){
    while(1){
        size_t left = LEFT(i);
        size_t right = RIGHT(i);
        size_t smallest = i;

        if(left < h->size && h->compare(h->items[left], h->items[smallest]) < 0){
            smallest = left;
        }
        /* the right child may not exist, so check right < size first */
        if(right < h->size && h->compare(h->items[right], h->items[smallest]) < 0){
            smallest = right;
        }
        if(smallest == i){
            break;
        }
        swap(&h->items[i], &h->items[smallest]);
        i = smallest;
    }
}

Heap *heap_create(HeapCompare compare){
    if(compare == NULL){
        return NULL;
    }
    Heap *h = malloc(sizeof(Heap));
    if(h == NULL){
        return NULL;
    }
    h->items = malloc(HEAP_START_CAPACITY * sizeof(void *));
    if(h->items == NULL){
        free(h);
        return NULL;
    }
    h->size = 0;
    h->capacity = HEAP_START_CAPACITY;
    h->compare = compare;
    return h;
}

int heap_push(Heap *h, void *data){
    if(h == NULL){
        return 0;
    }
    if(h->size == h->capacity){
        size_t new_capacity = h->capacity * 2;
        void **bigger = realloc(h->items, new_capacity * sizeof(void *));
        if(bigger == NULL){
            return 0;              /* old array is still valid, nothing lost */
        }
        h->items = bigger;
        h->capacity = new_capacity;
    }
    h->items[h->size] = data;      /* put the new item at the end... */
    h->size++;
    sift_up(h, h->size - 1);       /* ...then move it up to its place */
    return 1;
}

void *heap_pop(Heap *h){
    if(h == NULL || h->size == 0){
        return NULL;
    }
    void *smallest = h->items[0];
    h->size--;
    if(h->size > 0){
        h->items[0] = h->items[h->size];   /* last item goes to the top... */
        sift_down(h, 0);                   /* ...then moves down to its place */
    }
    return smallest;
}

void *heap_peek(const Heap *h){
    if(h == NULL || h->size == 0){
        return NULL;
    }
    return h->items[0];
}

size_t heap_size(const Heap *h){
    if(h == NULL){
        return 0;
    }
    return h->size;
}

void heap_clear(Heap *h){
    if(h != NULL){
        h->size = 0;
    }
}

void heap_destroy(Heap *h){
    if(h == NULL){
        return;
    }
    free(h->items);
    free(h);
}