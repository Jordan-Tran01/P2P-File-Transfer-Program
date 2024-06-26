#ifndef MERKLE_TREE_H
#define MERKLE_TREE_H

#include <stddef.h>
#include<chk/pkgchk.h>

#define SHA256_HEXLEN (64)

struct merkle_tree_node {
    void* key;
    void* value;
    struct merkle_tree_node* left;
    struct merkle_tree_node* right;
    int is_leaf;
    char expected_hash[SHA256_HEXLEN];
    char computed_hash[SHA256_HEXLEN];
};


struct merkle_tree {
    struct merkle_tree_node* root;
    size_t n_nodes;
};

struct merkle_tree_node* build_merkle_tree(struct chunk_obj* chunks, int start, int end);
void free_merkle_tree(struct merkle_tree_node* node);
int find_hash(struct merkle_tree_node* node, const char* hash);

#endif