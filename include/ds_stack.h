#ifndef DS_STACK_H
#define DS_STACK_H

#include <stddef.h>

/* A stack (LIFO) built on a growing array of pointers.
   The stack does NOT own the data it stores: the caller creates and frees it. */
typedef struct {
    void **items;      /* array of pointers to the stored items */
    size_t size;       /* how many items are in the stack */
    size_t capacity;   /* how many slots the array has */
} Stack;

Stack  *stack_create(void);
int     stack_push(Stack *s, void *data);   /* returns 1 on success, 0 on failure */
void   *stack_pop(Stack *s);                /* returns NULL if the stack is empty */
void   *stack_peek(const Stack *s);         /* top item without removing it, NULL if empty */
size_t  stack_size(const Stack *s);
void    stack_destroy(Stack *s);            /* frees the stack, not the stored data */

#endif