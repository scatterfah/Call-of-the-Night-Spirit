#include <stdio.h>
#include <assert.h>
#include "ds_stack.h"

int main(void) {
    Stack *s = stack_create();
    assert(s != NULL);
    assert(stack_size(s) == 0);
    assert(stack_pop(s) == NULL);
    assert(stack_peek(s) == NULL);
    printf("stack_create OK\n");

    int a = 1, b = 2, c = 3;
    stack_push(s, &a);
    stack_push(s, &b);
    stack_push(s, &c);
    assert(stack_size(s) == 3);
    assert(*(int *)stack_peek(s) == 3);
    assert(stack_size(s) == 3);              /* peek must not change size */
    assert(*(int *)stack_pop(s) == 3);
    assert(*(int *)stack_pop(s) == 2);
    assert(*(int *)stack_pop(s) == 1);
    assert(stack_pop(s) == NULL);
    assert(stack_size(s) == 0);
    printf("stack push/pop/peek OK\n");

    int nums[50];
    for(int i = 0; i < 50; i++){
        nums[i] = i;
        assert(stack_push(s, &nums[i]) == 1);
    }
    assert(stack_size(s) == 50);
    for(int i = 49; i >= 0; i--){
        assert(*(int *)stack_pop(s) == i);
    }
    assert(stack_size(s) == 0);
    printf("stack grow OK\n");

    stack_destroy(s);
    return 0;
}