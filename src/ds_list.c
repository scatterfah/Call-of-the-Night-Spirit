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