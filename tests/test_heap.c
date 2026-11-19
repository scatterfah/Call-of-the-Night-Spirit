#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "ds_heap.h"

/* compare plain ints: smallest number first */
static int cmp_int(const void *a, const void *b){
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);
}

/* a zombie-like struct, to show the same heap works for auto-aim */
typedef struct { float x, y; } Pos;
static Pos player = {0, 0};
static int cmp_distance(const void *a, const void *b){
    const Pos *p = a, *q = b;
    float da = hypotf(p->x - player.x, p->y - player.y);
    float db = hypotf(q->x - player.x, q->y - player.y);
    return (da > db) - (da < db);
}

int main(void){
    /* new heap and empty pop */
    Heap *h = heap_create(cmp_int);
    assert(h != NULL);
    assert(heap_size(h) == 0);
    assert(heap_pop(h) == NULL);
    assert(heap_peek(h) == NULL);
    assert(heap_create(NULL) == NULL);
    printf("heap_create OK\n");

    /* random numbers in, sorted numbers out (more than start capacity) */
    srand(42);
    enum { N = 1000 };
    int nums[N];
    for(int i = 0; i < N; i++){
        nums[i] = rand() % 500;          /* duplicates are guaranteed */
        assert(heap_push(h, &nums[i]) == 1);
    }
    assert(heap_size(h) == N);
    int prev = *(int *)heap_peek(h);
    assert(heap_size(h) == N);           /* peek must not change size */
    for(int i = 0; i < N; i++){
        int cur = *(int *)heap_pop(h);
        assert(cur >= prev);
        prev = cur;
    }
    assert(heap_size(h) == 0);
    assert(heap_pop(h) == NULL);
    printf("heap sorted pop + duplicates + growth OK\n");

    /* small hand-checked case from the guide */
    int v[] = {5, 2, 9, 3, 7};
    for(int i = 0; i < 5; i++) heap_push(h, &v[i]);
    assert(*(int *)heap_peek(h) == 2);
    int expected[] = {2, 3, 5, 7, 9};
    for(int i = 0; i < 5; i++) assert(*(int *)heap_pop(h) == expected[i]);
    printf("heap small case OK\n");

    /* heap_clear then reuse */
    for(int i = 0; i < 5; i++) heap_push(h, &v[i]);
    heap_clear(h);
    assert(heap_size(h) == 0 && heap_pop(h) == NULL);
    heap_push(h, &v[0]);
    assert(*(int *)heap_pop(h) == 5);
    heap_destroy(h);
    printf("heap_clear OK\n");

    /* auto-aim: closest zombie to the player comes out first */
    Heap *aim = heap_create(cmp_distance);
    Pos z[] = {{300, 400}, {10, 10}, {-50, 0}, {100, 100}, {0, 5}};
    for(int i = 0; i < 5; i++) heap_push(aim, &z[i]);
    assert(heap_pop(aim) == &z[4]);      /* (0,5)    */
    assert(heap_pop(aim) == &z[1]);      /* (10,10)  */
    assert(heap_pop(aim) == &z[2]);      /* (-50,0)  */
    assert(heap_pop(aim) == &z[3]);      /* (100,100)*/
    assert(heap_pop(aim) == &z[0]);      /* (300,400)*/
    heap_destroy(aim);
    printf("heap auto-aim OK\n");

    return 0;
}