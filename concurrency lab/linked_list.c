#include <stdlib.h>
#include "linked_list.h"
#include <stdbool.h>
#include <stdio.h>

/*
For a reminder on how typedef struct works watch this video:
https://www.youtube.com/watch?v=Bw3sUC6Txus 

printf("list->head: %p\n", list->head);
printf("(list->head->data): %p\n", (list->head->data));
printf("*(list->head->data): %d\n", *(int*)(list->head->data));

list_node_t *node = (list_node_t *)malloc(sizeof(list_node_t));
*/

// Creates and returns a new list
list_t* list_create()
{
    //we cast (list_t*) because old versions of C had malloc return a (char*) pointer
    list_t* new_list = (list_t*)malloc(sizeof(list_t));
    
    //since the list is empty, initialize the list head, tail, and count to NULL or 0
    new_list->head = NULL;
    new_list->tail = NULL;
    new_list->count = 0;
    
    return new_list;
}

// Destroys a list
void list_destroy(list_t* list)
{
    list_node_t* curr = list_head(list);

    //loop through the list and free every node
    while (curr != NULL) 
    {
        //list_node_t* next = curr->next;
        free(curr);
        curr = curr->next;
    }

    free(list); //free the allocated memory
    list = NULL; //set the pointer to NULL to avoid further use of it
}

// Returns head of the list
list_node_t* list_head(list_t* list)
{    
    return list->head;
}

// Returns tail of the list
list_node_t* list_tail(list_t* list)
{
    return list->tail;
}

// Returns next element in the list
list_node_t* list_next(list_node_t* node)
{
    return node->next;
}

// Returns prev element in the list
list_node_t* list_prev(list_node_t* node)
{
    return node->prev;
}

// Returns end of the list marker
list_node_t* list_end(list_t* list)
{
    /* IMPLEMENT THIS IF YOU WANT TO USE LINKED LISTS */
    return NULL;
}

// Returns data in the given list node
void* list_data(list_node_t* node)
{
    return node->data;
}

// Returns the number of elements in the list
size_t list_count(list_t* list)
{
    return list->count;
}

// Finds the first node in the list with the given data
// Returns NULL if data could not be found
list_node_t* list_find(list_t* list, void* data)
{
    list_node_t* curr = list_head(list);

    while (curr != NULL) 
    {
        if (curr->data == data)
        {
            return curr;
        }
        curr = list_next(curr);
    }
    return NULL;
}

// Inserts a new node in the list with the given data
// Returns new node inserted
list_node_t* list_insert(list_t* list, void* data)
{

    //to avoid duplicates
    if (list_find(list, data) != NULL)
    {
        return NULL;
    }

    //insert to end of list
    list_node_t* new_node = (list_node_t*)malloc(sizeof(list_node_t));
    new_node->next = NULL;
    new_node->prev = NULL;
    new_node->data = data;

    if (list_count(list) == 0) 
    {
        list->head = new_node;
        list->tail = new_node;
    }

    else 
    {
        new_node->prev = list->tail;
        list->tail->next = new_node;

        list->tail = new_node;
    }

    list->count++;
    return list_tail(list);
}

// Removes a node from the list and frees the node resources
void list_remove(list_t* list, void* data)
{
    list_node_t* remove_node = list_find(list, data);

    if (remove_node == NULL)
    {
        return;
    }
        
    else 
    {
        //if there's only one node in the list
        if (list_count(list) == 1)
        {
            list->head = NULL;
            list->tail = NULL;
        }

        else if (remove_node == list_head(list))
        {
            list->head = list->head->next;
            list->head->prev = NULL;
        }

        else if (remove_node == list_tail(list))
        {
            list->tail = list->tail->prev;
            list->tail->next = NULL;
        } 

        else 
        {
            remove_node->prev->next = remove_node->next;
            remove_node->next->prev = remove_node->prev;
        }  

        list->count--;
        free(remove_node);

    }
}