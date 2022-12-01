#include "kmalloc.h"
#include "lib.h"

#define DEBUG 0


block_desc_t malloc_heads[BINS];

/* void init_kmalloc
 * Description: Initializes kmalloc free list structures
 * Inputs: None
 * Return Value: None
 * Side Effects: modifies the malloc_head array
 */
void init_kmalloc(){
    // Must be atomic due to linked data structure
    uint32_t flags;
    cli_and_save(flags);

    uint32_t idx;

    for(idx = 0; idx < BINS; idx++){
        // Largest Bin
        if(idx == BINS - 1){
            (&malloc_heads[idx])->next = (block_desc_t*)MALLOC_START;
            (&malloc_heads[idx])->next->next = NULL;
            (&malloc_heads[idx])->next->size = (1 << (idx+MIN_BIN_LOG+1));
            (&malloc_heads[idx])->next->prev = (&malloc_heads[idx]);
            (&malloc_heads[idx])->prev = NULL;
            (&malloc_heads[idx])->size = (1 << (idx+MIN_BIN_LOG+1)); // 2^(idx+BINS+1), +1 for zero indexing
        }
        
        // All other bins
        else {
            (&malloc_heads[idx])->next = NULL;
            (&malloc_heads[idx])->prev = NULL;
            (&malloc_heads[idx])->size = (1 << (idx+MIN_BIN_LOG+1)); // 2^(idx+BINS+1), +1 for zero indexing
        }
    }

    restore_flags(flags);
}

/* void print_lists
 * Description: Print the free list structure
 * Inputs: None
 * Return Value: None
 * Side Effects: None
 */
void print_lists(){
    uint32_t idx;

    for(idx = 0; idx < BINS; idx++){
        printf("%d.) Size: %d --> ", idx, (1 << (idx+MIN_BIN_LOG+1)));

        block_desc_t* head = (&malloc_heads[idx])->next;

        while(head != NULL){
            printf("X --> ");
            head = head->next;
        }

        printf("NULL\n");
    }
}

/* uint32_t int_log2(uint32_t val)
 * Description: print the rounded down log base 2 of the input
 * Inputs: uint32_t val = val to take log of
 * Return Value: log of val
 * Side Effects: None
 */
uint32_t int_log2(uint32_t val){
    if(val == 0){
        // Cause a divide by zero because log2(0) is undefined
        val = 1 / val;
    }

    uint32_t pow = 0;
    while(val != 1){
        val /= 2; // divide the block in half
        pow++;
    }

    return pow;
}

/* block_desc_t* get_bin_head(uint32_t size)
 * Description: get the head of the doubly linked list corresponding
                to the bin of size passed
 * Inputs: uint32_t size = the size to get the head of
 * Return Value: the block_desc corresponding to the head
 * Side Effects: None
 */
block_desc_t* get_bin_head(uint32_t size){
    return &(malloc_heads[int_log2(size) - MIN_BIN_LOG - 1]); // -1 for zero indexing
}

/* void remove_from_list(block_desc_t* node)
 * Description: remove the block from the free doubly linked list
 * Inputs: the node to remove
 * Return Value: None
 * Side Effects: Changes the free list
 */
void remove_from_list(block_desc_t* node){
    if(node == NULL || node->prev == NULL){
        return;
    }

    block_desc_t* next = node->next;

    node->prev->next = next;

    if(next != NULL){
        next->prev = node->prev;
    }
}

/* void add_to_list(block_desc_t* node)
 * Description: add the block from the free doubly linked list
 * Inputs: the node to add
 * Return Value: None
 * Side Effects: Changes the free list
 */
void add_to_list(block_desc_t* node){
    block_desc_t* head = get_bin_head(node->size);
    block_desc_t* next = head->next;

    if(next != NULL){
        next->prev = node;
    }

    head->next = node;
    node->prev = head;
    node->next = next;
}

/* void void* split_block(block_desc_t* node)
 * Description: Split the block into two smaller sized blocks
 * Inputs: the node to split
 * Return Value: None
 * Side Effects: Changes the free list
 */
void* split_block(block_desc_t* node){
    block_desc_t* new_node = (block_desc_t*)(((uint32_t)(node)) + (node->size/2));

    remove_from_list(node);

    node->size /= 2;
    new_node->size = node->size;

    add_to_list(node);
    add_to_list(new_node);

    return new_node;
}

/* block_desc_t* get_partner(block_desc_t* node)
 * Description: Obtain a pointer to the given blocks "buddy"
 * Inputs: the node to get the buddy of
 * Return Value: the buddy
 * Side Effects: None
 */
block_desc_t* get_partner(block_desc_t* node){
    uint32_t size = node->size;
    uint32_t loc = (uint32_t)(node);
    loc = loc ^ size;
    block_desc_t* partner = (block_desc_t*)(loc);

    return partner;
}

/* uint8_t isfree(block_desc_t* node)
 * Description: check if the given node is free
 * Inputs: the node to check
 * Return Value: 1 if free, 0 if not
 * Side Effects: None
 */
uint8_t isfree(block_desc_t* node){
    block_desc_t* head = get_bin_head(node->size)->next;

    while(head != NULL){
        if(((uint32_t)(node)) == ((uint32_t)(head))){
            return 1;
        }
        head = head->next;
    }

    return 0;
}

/* void try_join_partners(block_desc_t* node)
 * Description: try to recursively join the nodes upward starting at the given node
 * Inputs: the node to join up from
 * Return Value: None
 * Side Effects: Changes the free list
 */
void try_join_partners(block_desc_t* node){
    block_desc_t* curr = node;
    while(curr->size < MALLOC_SPACE){
        block_desc_t* partner = get_partner(curr);

        if(!isfree(partner)){
            return;
        }

        block_desc_t* bigger = (((uint32_t) curr) < ((uint32_t) partner)) ? curr : partner;
        
        remove_from_list(curr);
        remove_from_list(partner);

        bigger->size *= 2;

        add_to_list(bigger);

        #if DEBUG
        print_lists();
        // getc(stdin);
        #endif

        curr = bigger;
    }
}

/* void* kmalloc(uint32_t size)
 * Description: malloc a node of at least size bytes
 * Inputs: size, number of bytes needed
 * Return Value: pointer to start of malloced block
 * Side Effects: Changes the free list
 */
void* kmalloc(uint32_t size){
    // Must be atomic due to linked data structure
    uint32_t flags;
    cli_and_save(flags);
    

    if(size == 0){
        return NULL;
    }

    uint8_t need_to_split;
    uint32_t curr_size;
    kmalloc_start:
    need_to_split = 0;
    curr_size = MIN_SIZE;
    while (curr_size <= MALLOC_SPACE){
        if(need_to_split == 1){
            block_desc_t* block = get_bin_head(curr_size)->next;
            if(block != NULL){
                split_block(block);
                // Label used so that we are not doing recursion in the kernel
                goto kmalloc_start;
            }
        }
        else if((curr_size - sizeof(block_desc_t)) >= size){
            block_desc_t* head = get_bin_head(curr_size);

            // If free list is empty
            if(head->next == NULL){
                need_to_split = 1;
            } else {
                block_desc_t* ret = head->next;
                remove_from_list(ret);
                
                restore_flags(flags);
                return ((void*)(ret)) + sizeof(block_desc_t);
            }
        }

        curr_size *= 2;
    }

    restore_flags(flags);
    return NULL;
}

/* void kfree(void* mem)
 * Description: free the malloced node and then try to rejoin nodes
 * Inputs: pointer to start of block to be freed
 * Return Value: None
 * Side Effects: Changes the free list
 */
void kfree(void* mem){
    // Must be atomic due to linked data structure
    uint32_t flags;
    cli_and_save(flags);

    if(mem == NULL){
        return;
    }

    block_desc_t* node = (block_desc_t*)(mem - sizeof(block_desc_t));

    // Only proceed if not already free
    if(isfree(node)){
        return;
    }

    add_to_list(node);

    try_join_partners(node);

    restore_flags(flags);
}
