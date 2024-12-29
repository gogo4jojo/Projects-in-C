/*
mm.c

Name: Josiah Kutai
 
My code is an implementation of memory allocation. It uses malloc(), free(), and realloc().
Malloc() allocates memory in the heap. Free() frees memory in the heap. Realloc() reallocates memory in the heap.
I use a segregated free list in my code that is made up of 14 free lists. Below are how the seg lists are organized:

class           size of free block (bytes)
0:              32
1:              48
2:              64 - 127
3:              128 - 255
4:              256 - 511
5:              512 - 1023 
6:              1024 - 2047
7:              2048 - 4095
8:              4096 - 8191
9:              8192 - 16,383
10:             16,384 - 32,767
11:             32,768 - 65,535
12:             65,536 - 131,071
13:             >= 131,072

Every free block has a pointer to the previous and next free block.
The last significant bit (LSB) of a block tells us if the block is allocated or free. (1 = alloc, 0 = free)
The 2nd LSB tells us if the prev block is allocated or free.

My code uses footer optimization so the allocated blocks do not have footers. We are able to tell if the previous block
is allocated or not through the prev alloc bit.

The goal of this assignment is to develop effecient memory allocation. 


 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#include <math.h>

#include "mm.h"
#include "memlib.h"


/*
 * If you want to enable your debugging output and heap checker code,
 * uncomment the following line. Be sure not to have debugging enabled
 * in your final submission.
 */
//#define DEBUG

#ifdef DEBUG
// When debugging is enabled, the underlying functions get called
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#else
// When debugging is disabled, no code gets generated
#define dbg_printf(...)
#define dbg_assert(...)
#endif // DEBUG

// do not change the following!
#ifdef DRIVER
// create aliases for driver tests
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mm_memset
#define memcpy mm_memcpy
#endif // DRIVER

#define ALIGNMENT 16

#define WORD_SIZE 8
#define MIN_FREE_BLOCK_SIZE 32
#define PREV_BLOCK_SIZE 8
#define NEXT_BLOCK_SIZE 8
#define HEADER_SIZE 8
#define FOOTER_SIZE 8
#define EPILOGUE_SIZE 8

uint64_t get_total_block_size(uint64_t* block_ptr);
void display_heap();
void display_seg_list();
uint64_t* get_next_free_block(uint64_t* ptr);
uint64_t* get_prev_free_block(uint64_t* ptr);
void set_prev_free_block(uint64_t* ptr, uint64_t addr);
void set_next_free_block(uint64_t* ptr, uint64_t addr);


uint64_t* prologue; //pointer to the prologue header
uint64_t* epilogue; //pointer to the epilogue

uint64_t* seg_list[14]; //list contains address of pointers to free blocks


uint64_t get_class(uint64_t size) 
{
    uint64_t x = log2(size);

    if (size == 32)
    {
        return 0;
    }

    else if (size == 48)
    {
        return 1;
    }

    else if (x <= 6)
    {
        return 2;
    }

    else if (x <= 17)
    {
        return x - 5;
    }
    
    else
    {
        return 13;
    }
    
}

//deletes a block from seg list
void delete_seg_list(uint64_t* block) {

    //get the class of the block
    uint64_t class = get_class(get_total_block_size(block));

    uint64_t* prev_block = get_prev_free_block(block);
    uint64_t* next_block = get_next_free_block(block);

    if (prev_block != NULL) {
        set_next_free_block(prev_block, (uint64_t)next_block);
    }
    
    if (next_block != NULL) {
        set_prev_free_block(next_block, (uint64_t)prev_block);
    }

    
    if (block == seg_list[class]) {
        seg_list[class] = next_block;
    }

}

void insert_seg_list(uint64_t* block)
{
    uint64_t class = get_class(get_total_block_size(block));
    uint64_t** list_head = &seg_list[class];

    if (*list_head == NULL) 
    {
        *list_head = block;
        set_next_free_block(block, 0);
        set_prev_free_block(block, 0);
    } 

    else 
    {
        uint64_t* old_ptr = *list_head;
        *list_head = block;

        set_next_free_block(*list_head, (uint64_t)old_ptr);
        set_prev_free_block(*list_head, 0);

        set_prev_free_block(old_ptr, (uint64_t)(*list_head)); 
    }
}

void reset_seg_list() {
    for (int i = 0; i < 14; i++) {
        seg_list[i] = NULL;
    }
}

uint64_t get_total_block_size(uint64_t* block_ptr)
{
    return (*(block_ptr) & 0xFFFFFFFFFFFFFFF0);
}

uint64_t is_block_allocated(uint64_t* block_ptr)
{
    return (*(block_ptr) & 0x0000000000000001);
}

void set_prev_bit(uint64_t* block_ptr, uint64_t value)
{
    *block_ptr = (*block_ptr) | value;
}

//if prev bit is zero, that means the prev block is free
void set_prev_bit_to_zero(uint64_t* block_ptr)
{
    *block_ptr = (*block_ptr) & 0xFFFFFFFFFFFFFFF1;
}

uint64_t get_prev_bit(uint64_t* block_ptr)
{
    return (*(block_ptr) & 0x0000000000000002);
}


uint64_t bytes_to_words(uint64_t size) 
{ 
    return (size / 8);
}

uint64_t* get_next_block(uint64_t* block_ptr)
{
    uint64_t num_words = bytes_to_words(get_total_block_size(block_ptr));
    return (block_ptr + num_words);
}



//paylod_ptr should point to allocated block (right after the header)
void* get_payload_ptr(uint64_t* block_ptr)
{
    return (block_ptr + 1);
}

void set_block_value (uint64_t* pointer, uint64_t value) 
{
    *(pointer) = value;
    return;
}


//increase pointer by words
uint64_t* incr_pointer(uint64_t *pointer, uint64_t words) 
{ 
    return pointer + words;
}

//decrease pointer by words
uint64_t* decr_pointer(uint64_t *pointer, uint64_t words) 
{ 
    return pointer - words;
}

uint64_t* head_to_foot(uint64_t *pointer) 
{ 
    return incr_pointer(pointer, bytes_to_words(get_total_block_size(pointer)) - 1);
}

uint64_t* foot_to_head(uint64_t *pointer) 
{ 
    return decr_pointer(pointer, bytes_to_words(get_total_block_size(pointer)) - 1);
}

uint64_t* get_prev_block(uint64_t* block_ptr)
{
    block_ptr = decr_pointer(block_ptr, 1);
    block_ptr = foot_to_head(block_ptr);
    return block_ptr;
}

uint64_t* get_next_free_block(uint64_t* ptr) 
{
    ptr = incr_pointer(ptr, 2);

    if (*ptr == 0) {
        return NULL;
    }
    
    uint64_t next_addr = *ptr;                  //get the address 
    uint64_t* next_ptr = (uint64_t*)next_addr;             
    
    return next_ptr;
}

uint64_t* get_prev_free_block(uint64_t* ptr) 
{
    ptr = incr_pointer(ptr, 1);

    if (*ptr == 0) {
        return NULL;
    }
    
    uint64_t prev_addr = *ptr;                   
    uint64_t* prev_ptr = (uint64_t*)prev_addr;             
    
    return prev_ptr;
}

void set_next_free_block(uint64_t* ptr, uint64_t addr) 
{
    ptr = incr_pointer(ptr, 2);
    *ptr = addr;
}

void set_prev_free_block(uint64_t* ptr, uint64_t addr) 
{
    ptr = incr_pointer(ptr, 1);
    *ptr = addr;
}

//splits a free block and allocates some of its space
void split_and_allocate_block(uint64_t* block_ptr, uint64_t size)
{
    uint64_t* old_header = block_ptr;
    delete_seg_list(old_header);

    //block_ptr should point to free block, first get the size
    uint64_t old_size = get_total_block_size(block_ptr);

    //change the block header to new size, the +1 is for the alloc bit
    if (get_prev_bit(block_ptr) == 0) {
        set_block_value(block_ptr, size+1); 
    } else {
        set_block_value(block_ptr, size+1);
        set_prev_bit(block_ptr, 2); //2 = 0x10
    }

    //move to block "footer"
    block_ptr = head_to_foot(block_ptr);

    uint64_t difference_size = old_size - size;

    
    if (difference_size >= MIN_FREE_BLOCK_SIZE) {
        //change the new free block header
        block_ptr = incr_pointer(block_ptr, 1);
        set_block_value(block_ptr, difference_size);
        set_prev_bit(block_ptr, 2);

        insert_seg_list(block_ptr);

        //change the new free block footer
        block_ptr = head_to_foot(block_ptr);
        set_block_value(block_ptr, difference_size);
    } 
    else {
        block_ptr = incr_pointer(block_ptr, 1);
        set_prev_bit(block_ptr, 2);
    }

}

//this is for when we extend the heap to allocate space
void malloc_helper (uint64_t* ptr, uint64_t total_block_size) {
    /*
        at the beginning we are pointing to the payload or block space
        so we need to move back by one word to go to our new header
        which is currently the epilogue
    */

    uint64_t* pointer = ptr;

    pointer = decr_pointer(pointer, 1);
    set_block_value(pointer, total_block_size + 1); //the +1 is for the alloc bit
    set_prev_bit(pointer, 2);
    
    //go to epilogue
    pointer = get_next_block(pointer);
    set_block_value(pointer, 0x1);
    set_prev_bit(pointer, 2);

    //set epilogue pointer
    epilogue = pointer;
    return;
}


void coalesce(uint64_t* prev_pointer, uint64_t* curr_pointer) 
{
    //SIKE THE PROBLEM IS ACTUALLY HERE
    uint64_t prev_size = get_total_block_size(prev_pointer);
    uint64_t curr_size = get_total_block_size(curr_pointer);

    uint64_t total_size = prev_size + curr_size;

    if (get_prev_bit(prev_pointer) == 0) {
        set_block_value(prev_pointer, total_size);
    } else {
        set_block_value(prev_pointer, total_size);
        set_prev_bit(prev_pointer, 2);
    }
    
    prev_pointer = head_to_foot(prev_pointer);
    set_block_value(prev_pointer, total_size);

}

//inserts a new block into the free list but finds the best fit
uint64_t* best_fit(uint64_t new_size) {
    uint64_t* best_block = NULL;

    uint64_t class = get_class(new_size);

    uint64_t* curr = seg_list[class];
    uint64_t best_size_diff = 0xFFFFFFFFFFFFFFFF; 
    uint64_t curr_size_diff = 0;

    int count = 0;


    while (curr != NULL || class < 14) {

        if (count > 17) {
            break;
        }

        if (curr == NULL && class < 13) {
            class += 1;
            curr = seg_list[class];
            continue;
        } 

        else if (curr == NULL && class == 13) {
            break;
        }

        if (get_total_block_size(curr) >= new_size) {
            curr_size_diff = get_total_block_size(curr) - new_size;
            if ( (curr_size_diff == 0 || curr_size_diff >= 32) && curr_size_diff < best_size_diff) {
                best_block = curr;
                best_size_diff = curr_size_diff;
                //break;
            }
        }
        
        if (curr != NULL) {
            curr = get_next_free_block(curr);
        }
        count += 1;
    }

    return best_block;
}


// rounds up to the nearest multiple of ALIGNMENT
static size_t align(size_t x)
{
    return ALIGNMENT * ((x+ALIGNMENT-1)/ALIGNMENT);
}

uint64_t* expand_heap(uint64_t new_block_size)
{
    return (uint64_t*)(mm_sbrk(new_block_size));
}

uint64_t* push_heap(uint64_t size) {
    //check if the last block of the heap is free, so that we only increase the heap size by the necessary amount
    uint64_t* ptr;
    if (get_prev_bit(epilogue) == 0) {
        ptr = get_prev_block(epilogue);
    } else {
        return NULL;
    }

    uint64_t old_size = get_total_block_size(ptr);
    if (is_block_allocated(ptr) == 0 && (size > old_size)) 
    {
        uint64_t* header = ptr;
        delete_seg_list(ptr);
        // (size - old_size) = the amount of free space we still need
        expand_heap(size - old_size);       //expand heap 

        if (get_prev_bit(ptr) == 0) {
            set_block_value(ptr, size + 1);     //set header value
        } else {
            set_block_value(ptr, size + 1);
            set_prev_bit(ptr, 2);
        }
        

        ptr = head_to_foot(ptr);            //move to footer
        // set_block_value(ptr, size + 1);     //set footer value

        ptr = incr_pointer(ptr, 1);         //move to epilogue
        set_block_value(ptr, 0x1);          //set epilogue value
        set_prev_bit(ptr, 2);
        epilogue = ptr;                     //set epilogue pointer

        ptr = header;
        //ptr = get_prev_block(ptr);          //move back to the new alloc block       

        return ptr;
    }
    
    return NULL;
}

/*
 * mm_init: returns false on error, true on success.
 */
bool mm_init(void)
{
    // IMPLEMENT THIS

    //this will increase the heap size by 32 bytes and return a pointer to the beginning of the heap
    uint64_t* pointer = expand_heap(WORD_SIZE + HEADER_SIZE + FOOTER_SIZE + EPILOGUE_SIZE);
    
    if (pointer == (uint64_t*) -1) {
        return false;
    } 
    else {
        
        //prologue header
        pointer = incr_pointer(pointer, 1);
        set_block_value(pointer, 0x11);

        //set prologue pointer
        prologue = pointer;                       
        
        //prologue footer
        pointer = incr_pointer(pointer, 1);                                  
        set_block_value(pointer, 0x11);                       
        
        //epilogue
        pointer = incr_pointer(pointer, 1);
        set_block_value(pointer, 0x01);     

        //set epilgoue pointer
        epilogue = pointer; 

        //mm_checkheap(__LINE__);
        return true;
    }
}

/*
 * malloc
 */
void* malloc(size_t size)
{
    //this means the heap was reset in memlib.c so we need to reset the seg_list as well
    if (mem_heapsize() == 32) {
        reset_seg_list();
    }

    uint64_t total_block_size = (uint64_t)align(HEADER_SIZE + size);    
    
    if (total_block_size < 32) {
        total_block_size = 32;
    }
    
    uint64_t* ptr = best_fit(total_block_size);
    
    if (ptr != NULL) {
        split_and_allocate_block(ptr, total_block_size);
        mm_checkheap(__LINE__);
        return get_payload_ptr(ptr);
    }

    ptr = push_heap(total_block_size); 

    if (ptr != NULL) {
        mm_checkheap(__LINE__);
        return get_payload_ptr(ptr);
    } 

    else {
        ptr = expand_heap(total_block_size);
        malloc_helper(ptr, total_block_size);
        mm_checkheap(__LINE__);
        return ptr;
    } 
}

/*
 * free
 */
void free(void* ptr)
{
    uint64_t* curr_block = (uint64_t*)ptr;
    curr_block = decr_pointer(curr_block, 1); //move back to block header

    uint64_t* prev_block = NULL;
    if (get_prev_bit(curr_block) == 0) {
        prev_block = get_prev_block(curr_block);
    }
    
    uint64_t* next_block = get_next_block(curr_block);
    set_prev_bit_to_zero(next_block);

    //set new block value for the header
    if (get_prev_bit(curr_block) == 0) {
        set_block_value(curr_block, get_total_block_size(curr_block));
    } else {
        set_block_value(curr_block, get_total_block_size(curr_block));
        set_prev_bit(curr_block, 2);
    }
    
    uint64_t size = get_total_block_size(curr_block);

    //curr_block is at the footer
    curr_block = head_to_foot(curr_block);

    //set new block value for the footer
    set_block_value(curr_block, size);

    //move curr_block back to header
    curr_block = foot_to_head(curr_block);


    if (prev_block != NULL && is_block_allocated(prev_block) == 0) {
        delete_seg_list(prev_block);
        
        coalesce(prev_block, curr_block);
        
        insert_seg_list(prev_block);
        
                
        if (is_block_allocated(next_block) == 0){
            
            uint64_t* next_next = get_next_block(next_block);
            set_prev_bit_to_zero(next_next);

            delete_seg_list(prev_block);
            delete_seg_list(next_block);
            
            coalesce(prev_block, next_block);
            insert_seg_list(prev_block);
            
        }
    }
    
    else if (is_block_allocated(next_block) == 0){
        uint64_t* next_next = get_next_block(next_block);
        set_prev_bit_to_zero(next_next);

        delete_seg_list(next_block);
        
        coalesce(curr_block, next_block);
        insert_seg_list(curr_block);
    }

    else {
        insert_seg_list(curr_block);
    }

    mm_checkheap(__LINE__);
    return; 
}

/*
 * realloc
 */
void* realloc(void* oldptr, size_t size)
{
    uint64_t* pointer = (uint64_t*)oldptr;

    if ( oldptr == NULL) {
        pointer = malloc(size);
    } 

    else if (size == 0) {
        free(oldptr);
        return NULL;
    } 

    else {
        
        //move pointer to the header of the block
        pointer = decr_pointer(pointer, 1);

        //align the size to 16
        uint64_t new_size = (uint64_t)align(size);

        //get size of the block, don't include the header or footer
        uint64_t old_size = get_total_block_size(pointer) - 8;
        uint64_t num_words = bytes_to_words(old_size);

        uint64_t* old_mem = calloc(num_words, WORD_SIZE);

        //move forward to allocated area
        pointer = incr_pointer(pointer, 1);


        //copy the bytes into old_mem
        mm_memcpy(old_mem, pointer, old_size);

        //what if i free the oldptr and then call split and allocate if the size is big enough to save time
        //free the old allocation
        free(oldptr);

        //new allocation
        pointer = malloc(new_size);

        //copy the old bytes into the new allocation

        if (old_size > new_size) {
            mm_memcpy(pointer, old_mem, new_size);
        } 

        else if (old_size <= new_size) {
            mm_memcpy(pointer, old_mem, old_size);
        }

    }

    mm_checkheap(__LINE__);
    return pointer;
}

/*
 * calloc
 * This function is not tested by mdriver, and has been implemented for you.
 */
void* calloc(size_t nmemb, size_t size)
{
    void* ptr;
    size *= nmemb;
    ptr = malloc(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

/*
 * Returns whether the pointer is in the heap.
 * May be useful for debugging.
 */
static bool in_heap(const void* p)
{
    return p <= mm_heap_hi() && p >= mm_heap_lo();
}

/*
 * Returns whether the pointer is aligned.
 * May be useful for debugging.
 */
static bool aligned(const void* p)
{
    size_t ip = (size_t) p;
    return align(ip) == ip;
}

/*
 * mm_checkheap
 * You call the function via mm_checkheap(__LINE__)
 * The line number can be used to print the line number of the calling
 * function where there was an invalid heap.
 */

void display_heap() {
    uint64_t* ptr = prologue;
    printf("\n");
    printf("\nDISPLAY HEAP\n");
    printf("Prologue: %p\n", prologue);
    for (ptr = prologue; get_total_block_size(ptr) > 0; ptr = get_next_block(ptr)) {
        printf("header: %p\n", ptr);
        printf("value: %lu\n", *ptr);
        printf("\n");
        if (is_block_allocated(ptr) == 0) {
            printf("footer: %p\n", head_to_foot(ptr));
            printf("value: %lu\n", *head_to_foot(ptr));
            printf("\n");
        }
    }
    printf("Epilogue: %p\n", epilogue);
    printf("value: %lu\n", *epilogue);
}

void display_seg_list() {
    printf("\n");
    for (int i = 0; i < 13; i++) {
        printf("seg_list[%d]: %p\n", i, seg_list[i]);
    }
     printf("\n");
}

bool in_seg_list(uint64_t* ptr)
{
    uint64_t* curr = NULL;

    for (int i = 0; i < 13; i++) {

        if (curr == NULL) 
        {
            curr = seg_list[i];
        }

        while (curr != NULL)
        {
            if (curr == ptr) 
            {
                return true;
            }
            curr = get_next_free_block(curr);
        }

    }
    
    return false;
}



bool mm_checkheap(int line_number)
{
#ifdef DEBUG
    // checking every block in the heap to see if it is aligned
    uint64_t* ptr = prologue;
    uint64_t size = 0;
    uint64_t* size_ptr = NULL;


    //first we check the prologue to see if it's aligned
    size = get_total_block_size(ptr);
    size_ptr = &size;
    if (!aligned(size_ptr))
    {
        printf("\nFailed at line number: %d\n", line_number);
        printf("%p (prologue) is not aligned, it's value is %lu\n", ptr, *ptr);
        return false;
    }

    //check if prologue is the correct size
    if (size != 16) 
    {
        printf("\nFailed at line number: %d\n", line_number);
        printf("%p (prologue) is not of size 16, it's value is %lu\n", ptr, *ptr);
        return false;
    }

    
    for (ptr = get_next_block(prologue); get_total_block_size(ptr) > 0; ptr = get_next_block(ptr)) 
    {
        size = get_total_block_size(ptr);
        size_ptr = &size;

        //check if block is aligned
        if (!aligned(size_ptr)) 
        {
            printf("\nFailed at line number: %d\n", line_number);
            printf("%p is not aligned, it's value is %lu\n", ptr, *ptr);

            return false;
        }

        //check if allocated block is in seg list
        if (is_block_allocated(ptr) == 1) 
        {
            if (in_seg_list(ptr)) 
            {
                printf("\nFailed at line number: %d\n", line_number);
                printf("%p is allocated and in the seg list, it's value is %lu\n", ptr, *ptr);
                return false;
            }
        }

        //check if header and footer of free blocks have the same size
        if (is_block_allocated(ptr) == 0) 
        {
            if (get_total_block_size(ptr) != get_total_block_size(head_to_foot(ptr)))
            {
                printf("\nFailed at line number: %d\n", line_number);
                printf("%p does not have the same size as it's footer, it's value is %lu\n", ptr, *ptr);
                return false;
            }
        }

        //check if block is in the heap
        if (!in_heap(ptr)) 
        {
            printf("\nFailed at line number: %d\n", line_number);
            printf("%p is not in the heap, it's value is %lu\n", ptr, *ptr);
            return false;
        }
    }

    //check if epilogue is aligned
    size = get_total_block_size(epilogue);
    size_ptr = &size;
    if (!aligned(size_ptr))
    {
        printf("\nFailed at line number: %d\n", line_number);
        printf("%p (epilogue) is not aligned, it's value is %lu\n", ptr, *ptr);
        return false;
    }


#endif // DEBUG
    return true;
}