#ifndef DS_LIST_H
#define DS_LIST_H

#include <stddef.h>

/* One node = one item in the list (for example one zombie). */
typedef struct ListNode {
    void *data;              /* points to the item stored here */
    struct ListNode *prev;   /* the node before this one */
    struct ListNode *next;   /* the node after this one */
} ListNode;

/* The list itself. */
typedef struct {
    ListNode *head;          /* first node (NULL if the list is empty) */
    ListNode *tail;          /* last node  (NULL if the list is empty) */
    size_t size;             /* how many nodes are in the list */
} List;

/* Functions. You will write these in src/ds_list.c. */
List     *list_create(void);
ListNode *list_push_back(List *list, void *data);
ListNode *list_push_front(List *list, void *data);
void     *list_remove(List *list, ListNode *node);
size_t    list_size(const List *list);
void      list_destroy(List *list);

#endif