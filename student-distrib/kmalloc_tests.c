#include "kmalloc.h"
#include "lib.h"
#include "kmalloc_tests.h"


void inorder(tnode_t* root){
    if (root != NULL) {
        inorder(root->left);
        printf("%d \n", root->key);
        inorder(root->right);
    }
}

tnode_t* search(tnode_t* root, int key){
    // Base Cases: root is null or key is present at root
    if (root == NULL || root->key == key)
       return root;
    
    // Key is greater than root's key
    if (root->key < key)
       return search(root->right, key);
 
    // Key is smaller than root's key
    return search(root->left, key);
}

tnode_t* newNode(int item){
    tnode_t* temp
        = (tnode_t*)kmalloc(sizeof(tnode_t));
    if(temp == NULL){
        printf("MALLOC RETURNED ERROR CODE");
    }
    temp->key = item;
    temp->left = temp->right = NULL;
    return temp;
}

tnode_t* insert(tnode_t* node, int key){
    /* If the tree is empty, return a new node */
    if (node == NULL)
        return newNode(key);
 
    /* Otherwise, recur down the tree */
    if (key < node->key)
        node->left = insert(node->left, key);
    else if (key > node->key)
        node->right = insert(node->right, key);
 
    /* return the (unchanged) node pointer */
    return node;
}


void free_tree(tnode_t* root){
    if(root == NULL){
        return;
    }

    free_tree(root->left);
    free_tree(root->left);

    kfree(root);
}


int run_km_tests(){

    void* mem1 = kmalloc(100);
    void* mem2 = kmalloc(100);
    void* mem3 = kmalloc(100);
    void* mem4 = kmalloc(100);
    void* mem5 = kmalloc(100);
    void* mem6 = kmalloc(100);

    kfree(mem1);
    kfree(mem4);
    kfree(mem3);
    kfree(mem5);
    kfree(mem6);
    kfree(mem2);

    return 0;
}
