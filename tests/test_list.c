#include <stdio.h>
#include <assert.h>
#include "ds_list.h"

int main(void) {
    List *l = list_create();
    assert(l != NULL);
    assert(list_size(l) == 0);
    assert(l->head == NULL);
    assert(l->tail == NULL);
    printf("list_create OK\n");
        int a = 10, b = 20, c = 30;
    list_push_back(l, &a);
    assert(list_size(l) == 1);
    assert(l->head == l->tail);

    list_push_back(l, &b);
    list_push_back(l, &c);
    assert(list_size(l) == 3);
    assert(*(int *)l->head->data == 10);
    assert(*(int *)l->tail->data == 30);
    assert(l->head->next->prev == l->head);
    assert(l->tail->prev->next == l->tail);
    printf("list_push_back OK\n");
        int d = 5;
    list_push_front(l, &d);
    assert(list_size(l) == 4);
    assert(*(int *)l->head->data == 5);
    assert(*(int *)l->head->next->data == 10);
    assert(l->head->next->prev == l->head);
    assert(*(int *)l->tail->data == 30);
    printf("list_push_front OK\n");
    return 0;
}