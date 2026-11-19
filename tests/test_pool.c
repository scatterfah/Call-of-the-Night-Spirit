#include <stdio.h>
#include <assert.h>
#include "ds_pool.h"

typedef struct { float x, y, vx, vy; } Bullet;

int main(void){
    assert(pool_create(0, 10) == NULL);
    assert(pool_create(sizeof(Bullet), 0) == NULL);

    Pool *p = pool_create(sizeof(Bullet), 5);
    assert(p != NULL);
    assert(pool_capacity(p) == 5);
    assert(pool_active_count(p) == 0);
    printf("pool_create OK\n");

    /* acquire until full, then the next acquire returns NULL */
    Bullet *b[5];
    for(int i = 0; i < 5; i++){
        b[i] = pool_acquire(p);
        assert(b[i] != NULL);
        assert(pool_active_count(p) == (size_t)(i + 1));
    }
    assert(pool_acquire(p) == NULL);
    assert(pool_active_count(p) == 5);
    printf("pool full OK\n");

    /* release one, acquire again: we get a slot back, and it is cleared */
    b[2]->x = 99; b[2]->vy = 7;
    assert(pool_release(p, b[2]) == 1);
    assert(pool_active_count(p) == 4);
    Bullet *again = pool_acquire(p);
    assert(again == b[2]);
    assert(again->x == 0 && again->vy == 0);
    assert(pool_active_count(p) == 5);
    assert(pool_acquire(p) == NULL);
    printf("pool release/reuse/clear OK\n");

    /* releasing twice must not break anything */
    assert(pool_release(p, b[1]) == 1);
    assert(pool_release(p, b[1]) == 0);
    assert(pool_active_count(p) == 4);
    Bullet *x = pool_acquire(p);
    Bullet *y = pool_acquire(p);
    assert(x == b[1]);
    assert(y == NULL);                        /* no second copy of the slot */
    printf("pool double release OK\n");

    /* bad pointers are rejected */
    Bullet outside;
    assert(pool_release(p, &outside) == 0);
    assert(pool_release(p, NULL) == 0);
    assert(pool_release(p, (char *)b[0] + 1) == 0);
    printf("pool invalid release OK\n");

    /* visiting active items with pool_get */
    pool_release(p, b[0]);
    pool_release(p, b[3]);
    size_t seen = 0;
    for(size_t i = 0; i < pool_capacity(p); i++){
        if(pool_get(p, i) != NULL) seen++;
    }
    assert(seen == pool_active_count(p));
    assert(seen == 3);
    printf("pool iterate OK\n");

    pool_destroy(p);
    return 0;
}