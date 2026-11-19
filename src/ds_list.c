#include<stdlib.h>
#include"ds_list.h"

List *list_create(void){
    List *list = malloc(sizeof(List));
    if(list == NULL){
        return NULL;
    }
    list -> head = NULL; 
    list -> tail = NULL;
    list -> size = 0;
    return list;
}

size_t list_size(const List *list){
    if(list == NULL){
        return 0;
    }
    return list -> size;
}

ListNode *list_push_back(List *list, void *data){
    if (list == NULL)
    return NULL;

    ListNode *node = malloc(sizeof(ListNode));
    if(node == NULL)
    return NULL;
    
    node -> data = data;
    node -> next = NULL;
    node -> prev = list ->tail;

    if(list -> tail == NULL){
        list -> head = node;
    }
    else{
        list -> tail -> next = node;
    }

    list -> tail = node;
    list -> size++;

    return node;
    
}
ListNode *list_push_front(List *list, void *data){
    if(list == NULL) 
    return NULL;

    ListNode *node = malloc(sizeof(ListNode));
    if(node == NULL) 
    return NULL;

    node -> data = data;
    node -> prev = NULL;
    node -> next = list -> head;

    if(list -> head == NULL){
        list -> tail = node;
    }
    else{
        list -> head -> prev = node;
    }

    list -> head = node;
    list -> size++;
    return node;
}
/* Frees the nodes and the list. It does NOT free the data the nodes point to. */
void list_destroy(List *list) {
    if (list == NULL) return;

    ListNode *node = list->head;
    while (node != NULL) {
        ListNode *next = node->next;    /* save next BEFORE freeing the node */
        free(node);
        node = next;
    }
    free(list);
}
/* Removes one node. Returns its data pointer so the caller can free it. */
void *list_remove(List *list, ListNode *node) {
    if (list == NULL || node == NULL) return NULL;

    void *data = node->data;

    if (node->prev == NULL) {
        list->head = node->next;        /* removing the first: head moves on */
    } else {
        node->prev->next = node->next;  /* neighbour before skips over it */
    }

    if (node->next == NULL) {
        list->tail = node->prev;        /* removing the last: tail moves back */
    } else {
        node->next->prev = node->prev;  /* neighbour after skips back over it */
    }

    free(node);
    list->size--;
    return data;
}