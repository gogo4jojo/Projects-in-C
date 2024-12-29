#include <stdlib.h>
#include "linked_list.h"
#include <stdbool.h>
#include <stdio.h>


//I reused some code from the concurrency lab

// Creates and returns a new list
list_t* list_create(compare_fn compare)
{
    list_t* new_list = (list_t*)malloc(sizeof(list_t));
    
    //since the list is empty, initialize the list head, tail, and count to NULL or 0
    new_list->head = NULL;
    new_list->tail = NULL;
    new_list->count = 0;
    new_list->compare = compare;
    
    return new_list;
}

// Destroys a list
void list_destroy(list_t* list)
{
    list_node_t* curr = list->head;

    //loop through the list and free every node
    while (curr != NULL) 
    {
        list_node_t* next = curr->next;
        free(curr);
        curr = next;
    }

    free(list); //free the allocated memory
    //list = NULL; //set the pointer to NULL to avoid further use of it
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

    //create node that we are inserting into the list
    list_node_t* new_node = (list_node_t*)malloc(sizeof(list_node_t));
    new_node->next = NULL;
    new_node->prev = NULL;
    new_node->data = data;

    //check if the list is empty
    if (list_count(list) == 0)
    {
        list->head = new_node;
        list->tail = new_node;
        list->count++;
        return new_node;
    }

    //if compare is NULL then insert to the head of list
    if (list->compare == NULL) 
    {
        new_node->next = list->head;
        list->head->prev = new_node;
        list->head = new_node;
        
        list->count++;
        return new_node;
    }


    //iterate through the list
    list_node_t* curr = list_head(list);
    while (curr != NULL)
    {
        if (list->compare(data, list_data(curr)) == 1)
        {
            curr = list_next(curr);
        }
        else 
        {
            if (curr == list_head(list))
            {
                list->head->prev = new_node;
                new_node->next = list->head;
                list->head = new_node;
            }
            else
            {

                curr->prev->next = new_node;
                new_node->prev = curr->prev;
                
                curr->prev = new_node;
                new_node->next = curr;
            }

            list->count++;
            return new_node;

        }
    }

    //if this line is reached, then we are inserting at the end of the list

    list->tail->next = new_node;
    new_node->prev = list->tail;

    list->tail = new_node;
    
    list->count++;
    return new_node;
    
}

// Removes a node from the list and frees the node resources
void list_remove(list_t* list, list_node_t* node)
{
    list_node_t* remove_node = list_find(list, node->data);

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

