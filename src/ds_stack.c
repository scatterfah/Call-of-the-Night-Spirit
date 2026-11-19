#include <stdlib.h>
#include "ds_stack.h"

#define STACK_START_CAPACITY 4

Stack *stack_create(void){
    Stack *s = malloc(sizeof(Stack));
    if(s == NULL){
        return NULL;
    }
    s->items = malloc(STACK_START_CAPACITY * sizeof(void *));
    if(s->items == NULL){
        free(s);
        return NULL;
    }
    s->size = 0;
    s->capacity = STACK_START_CAPACITY;
    return s;
}

int stack_push(Stack *s, void *data){
    if(s == NULL){
        return 0;
    }
    if(s->size == s->capacity){
        size_t new_capacity = s->capacity * 2;
        void **bigger = realloc(s->items, new_capacity * sizeof(void *));
        if(bigger == NULL){
            return 0;              /* old array is still valid, nothing lost */
        }
        s->items = bigger;
        s->capacity = new_capacity;
    }
    s->items[s->size] = data;
    s->size++;
    return 1;
}

void *stack_pop(Stack *s){
    if(s == NULL || s->size == 0){
        return NULL;
    }
    s->size--;
    return s->items[s->size];
}

void *stack_peek(const Stack *s){
    if(s == NULL || s->size == 0){
        return NULL;
    }
    return s->items[s->size - 1];
}

size_t stack_size(const Stack *s){
    if(s == NULL){
        return 0;
    }
    return s->size;
}

void stack_destroy(Stack *s){
    if(s == NULL){
        return;
    }
    free(s->items);
    free(s);
}