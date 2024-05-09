// merkletree.c
#include <tree/merkletree.h>
#include <crypt/sha256.h>
#include <chk/pkgchk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

struct merkle_tree_node* create_node(const char* hash) {
    struct merkle_tree_node* node = malloc(sizeof(struct merkle_tree_node));
    if (!node) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    if (!node->computed_hash) {
        perror("Memory allocation failed");
        free(node);
        exit(EXIT_FAILURE);
    }
    strncpy(node->computed_hash, hash, MAX_HASH_LEN);
    node->computed_hash[MAX_HASH_LEN] = '\0'; // Null-terminate the string
    node->left = NULL;
    node->right = NULL;
    node->is_leaf = 0;
    return node;
}

struct merkle_tree_node* build_merkle_tree(struct chunk_obj* chunks, int start, int end) {
    if (start > end) {
        return NULL;
    }
    if (start == end) {
        struct merkle_tree_node* leaf_node = create_node(chunks[start].hash);
        leaf_node->is_leaf = 1;
        return leaf_node;
    }

    int mid = (start + end) / 2;
    struct merkle_tree_node* left_child = build_merkle_tree(chunks, start, mid);
    struct merkle_tree_node* right_child = build_merkle_tree(chunks, mid + 1, end);

    char combined_hash[2 * MAX_HASH_LEN + 1];
    snprintf(combined_hash, sizeof(combined_hash), "%s%s", left_child->computed_hash, right_child->computed_hash);

    char parent_hash[MAX_HASH_LEN + 1];
    compute_hash(combined_hash, parent_hash);

    struct merkle_tree_node* parent_node = create_node(parent_hash);
    parent_node->left = left_child;
    parent_node->right = right_child;

    return parent_node;
}

void free_merkle_tree(struct merkle_tree_node* node) {
    if (node == NULL) {
        return;
    }
    free_merkle_tree(node->left);  // Free left subtree
    free_merkle_tree(node->right); // Free right subtree
    free(node);                    // Free the current node
}

void print_merkle_tree_hashes(struct merkle_tree_node* node) {
    if (node == NULL) {
        return;  // Base case: if the node is NULL, return.
    }

    // Recursive call to visit the left child
    print_merkle_tree_hashes(node->left);

    // Print the current node's hash value
    printf("Node Hash: %s\n", node->computed_hash);

    // Recursive call to visit the right child
    print_merkle_tree_hashes(node->right);
}