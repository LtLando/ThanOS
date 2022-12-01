#ifndef _KMALLOC_TESTS_H
#define _KMALLOC_TESTS_H

#include "types.h"

typedef struct tnode_t{
    uint32_t key;
    struct tnode_t* left;
    struct tnode_t* right;
} tnode_t;


int run_km_tests(void);
void free_tree(tnode_t* root);
tnode_t* insert(tnode_t* node, int key);
tnode_t* newNode(int item);
tnode_t* search(tnode_t* root, int key);
void inorder(tnode_t* root);

#endif
