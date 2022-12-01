#ifndef _MAIN_H
#define _MAIN_H

#include "types.h"

// 1 MB = 1048576
#define MALLOC_START 209715200
#define MALLOC_SPACE 1048576
#define MIN_SIZE 32
#define BINS 16 // Log2(MALLOC_SPACE) - 4 (-4 because 32 bytes is smallest block)
#define MIN_BIN_LOG 4

typedef struct block_desc_t{
    struct block_desc_t* prev;
    struct block_desc_t* next;
    uint32_t size;
} block_desc_t;


void* kmalloc(uint32_t size);
void kfree(void* mem);
void print_lists(void);
void remove_from_list(block_desc_t* node);
block_desc_t* get_bin_head(uint32_t size);
block_desc_t* get_partner(block_desc_t* node);
void join_partners(block_desc_t* node);
uint32_t int_log2(uint32_t val);
void init_kmalloc(void);


#endif
