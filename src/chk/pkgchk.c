#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <chk/pkgchk.h>
#include <crypt/sha256.h>
#include <tree/merkletree.h>
#include <unistd.h>
#include <stdbool.h>

// PART 1

/**
 * Loads the package for when a valid path is given
 *///
struct bpkg_obj* bpkg_load(const char* path) {
    FILE *bpkg_file = fopen(path, "r");

    if (!bpkg_file) {
        perror("Failed to open .bpkg file!");
        return NULL;
    }

    struct bpkg_obj* obj = malloc(sizeof(struct bpkg_obj));

    if (!obj) {
        perror("Failed to allocate memory for package object");
        fclose(bpkg_file);
        return NULL;
    }

    // Initialize obj fields
    memset(obj->ident, 0, sizeof(obj->ident)); // Ensure proper null termination
    memset(obj->filename, 0, sizeof(obj->filename)); // Ensure proper null termination
    obj->size = 0;
    obj->nhashes = 0;
    obj->nchunks = 0;

    char line[1030];
    while (fgets(line, sizeof(line), bpkg_file)) {
        if (strncmp(line, "ident:", 6) == 0) {
            char* ident_start = line + 6;
            strncpy(obj->ident, ident_start, sizeof(obj->ident) - 1);
            obj->ident[sizeof(obj->ident) - 1] = '\0';
        } else if (strncmp(line, "filename:", 9) == 0) {
            char* filename_start = line + 9;
            size_t len = strlen(filename_start);
            
            if (filename_start[len - 1] == '\n') {
                filename_start[len - 1] = '\0';
            }

            strncpy(obj->filename, filename_start, sizeof(obj->filename));
            obj->filename[sizeof(obj->filename) - 1] = '\0';
        } else if (strncmp(line, "size:", 5) == 0) {
            char* size_start = line + 5;

            obj->size = atoi(size_start);
        } else if (strncmp(line, "nhashes:", 8) == 0) {
            char* nhashes_start = line + 8;

            obj->nhashes = atoi(nhashes_start);
            
        } else if (strncmp(line, "nchunks:", 8) == 0) {
            char* nchunks = line + 8;

            obj->nchunks = atoi(nchunks);
        } else if (strncmp(line, "hashes:", 7) == 0) {
            // Assuming obj->nhashes has already been initialized
            obj->hashes = malloc(obj->nhashes * sizeof(char*));
            if (obj->hashes == NULL) {
                // Handle memory allocation failure
                fprintf(stderr, "Memory allocation failed for obj->hashes\n");
                exit(EXIT_FAILURE); // or return an error code, depending on your program structure
            }

            for (int i = 0; i < obj->nhashes; i++) {
                obj->hashes[i] = malloc((MAX_HASH_LEN + 1) * sizeof(char)); // Include space for null terminator

                if (obj->hashes[i] == NULL) {
                    // Handle memory allocation failure
                    fprintf(stderr, "Memory allocation failed for obj->hashes[%d]\n", i);
                    exit(EXIT_FAILURE); // or return an error code, depending on your program structure
                }

                if (fgets(line, sizeof(line), bpkg_file) != NULL) {
                    char *token = strtok(line, " \t\n");
                    if (token != NULL) {
                        // Skip leading whitespace
                        while (isspace(*token)) {
                            token++;
                        }
                        strncpy(obj->hashes[i], token, MAX_HASH_LEN);
                        obj->hashes[i][MAX_HASH_LEN] = '\0'; // Ensure null termination
                    } 
                } else {
                    i--; 
                    continue;
                }
            } 
        } else if (strncmp(line, "chunks:", 7) == 0) {
            obj->chunks = malloc(obj->nchunks * sizeof(struct chunk_obj));

            if (obj->chunks == NULL) {
                // Handle memory allocation failure
            }

            for (int i = 0; i < obj->nchunks; i++) {
                fgets(line, sizeof(line), bpkg_file);
                // Split the line into substrings using strtok
                char *token = strtok(line, ",");
                if (token != NULL) {
                    while (isspace(*token)) {
                        token++;
                    }
                    strcpy(obj->chunks[i].hash, token);
                    token = strtok(NULL, ",");
                    if (token != NULL) {
                        obj->chunks[i].offset = atoi(token);
                        token = strtok(NULL, ",");
                        if (token != NULL) {
                            obj->chunks[i].size = atoi(token);
                        }
                    }
                }
            }
        }
    }

    fclose(bpkg_file);

    return obj;
}

/**
 * Checks to see if the referenced filename in the bpkg file
 * exists or not.
 * @param bpkg, constructed bpkg object
 * @return query_result, a single string should be
 *      printable in hashes with len sized to 1.
 * 		If the file exists, hashes[0] should contain "File Exists"
 *		If the file does not exist, hashes[0] should contain "File Created"
 */

struct bpkg_query bpkg_file_check(struct bpkg_obj* bpkg) {
    struct bpkg_query query_result = { 0 };
    query_result.hashes = malloc(sizeof(char*));

    if (query_result.hashes == NULL) {
        perror("Memory allocation failed for hash string");
        exit(EXIT_FAILURE);
    }

    if (access(bpkg->filename, F_OK) == 0) {
        query_result.hashes[0] = strdup("File Exists");
    } else {
        query_result.hashes[0] = strdup("File Created");
    }

    if (query_result.hashes[0] == NULL) {
        perror("Memory allocation failed for hash string");
        exit(EXIT_FAILURE);
    }

    query_result.len = 1;
    return query_result;
}

void collect_hashes_inorder(struct merkle_tree_node* node, char*** hashes, size_t* index, size_t* current_capacity) {
    if (node == NULL) return;

    collect_hashes_inorder(node->left, hashes, index, current_capacity);

    // Ensure there is enough space to add a new hash
    if (*index >= *current_capacity) {
        // Increase the capacity
        *current_capacity *= 2;
        *hashes = realloc(*hashes, *current_capacity * sizeof(char*));
        if (*hashes == NULL) {
            perror("Memory reallocation failed for hashes");
            exit(EXIT_FAILURE);
        }
    }

    // Allocate memory and store the hash
    (*hashes)[*index] = malloc((MAX_HASH_LEN + 1) * sizeof(char));
    if ((*hashes)[*index] == NULL) {
        perror("Memory allocation failed for a hash string");
        exit(EXIT_FAILURE);
    }
    strcpy((*hashes)[*index], node->computed_hash);
    (*index)++;

    collect_hashes_inorder(node->right, hashes, index, current_capacity);
}

// void collect_min_hashes(struct merkle_tree_node* node, struct bpkg_query* qry, struct bpkg_query* complete_chunks) {
//     if (node == NULL) {
//         return;
//     }

//     if (node->is_leaf) {
//         if (is_chunk_complete(node, *complete_chunks)) {
//             printf("Correct leaf count: %ld\n", qry->capacity);
//         }
//     }

//     struct bpkg_query left_qry = {0}, right_qry = {0};
//     collect_min_hashes(node->left, &left_qry, complete_chunks);
//     collect_min_hashes(node->right, &right_qry, complete_chunks);
// }

void collect_min_hashes(struct merkle_tree_node* node, struct bpkg_query* qry, struct bpkg_query* complete_chunks) {
    if (node == NULL) {
        return;
    }

    // Initialize capacity if it's not set
    if (qry->capacity == 0) {
        qry->capacity = 10; // Initial capacity
        qry->hashes = malloc(qry->capacity * sizeof(char*));
        if (qry->hashes == NULL) {
            perror("Initial malloc failed for hashes");
            exit(EXIT_FAILURE);
        }
    }

    if (node->is_leaf) {
        if (is_chunk_complete(node, *complete_chunks)) {
            if (qry->len == qry->capacity) {
                qry->capacity *= 2;
                char** new_hashes = realloc(qry->hashes, qry->capacity * sizeof(char*));
                if (!new_hashes) {
                    perror("Failed to realloc memory for hashes in leaf");
                    exit(EXIT_FAILURE);
                }
                qry->hashes = new_hashes;
            }
            qry->hashes[qry->len++] = strdup(node->computed_hash);
        }
        return;
    }

    struct bpkg_query left_qry = {0}, right_qry = {0};
    collect_min_hashes(node->left, &left_qry, complete_chunks);
    collect_min_hashes(node->right, &right_qry, complete_chunks);

    if (left_qry.len == 1 && right_qry.len == 1) {
        if (strcmp(left_qry.hashes[0], node->left->computed_hash) == 0 && strcmp(right_qry.hashes[0], node->right->computed_hash) == 0) {
            if (qry->len == qry->capacity) {
                qry->capacity *= 2;
                char** new_hashes = realloc(qry->hashes, qry->capacity * sizeof(char*));
                if (!new_hashes) {
                    perror("Failed to realloc memory for hashes in non-leaf");
                    exit(EXIT_FAILURE);
                }
                qry->hashes = new_hashes;
            }
            qry->hashes[qry->len++] = strdup(node->computed_hash);
            bpkg_query_destroy(&left_qry);
            bpkg_query_destroy(&right_qry);
        } else {
            merge_queries(qry, &left_qry);
            merge_queries(qry, &right_qry);
        }
    } else {
        merge_queries(qry, &left_qry);
        merge_queries(qry, &right_qry);
    }
}

bool is_chunk_complete(struct merkle_tree_node* node, struct bpkg_query complete_qry) {
    for (size_t i = 0; i < complete_qry.len; i++) { 
        if (strcmp(node->computed_hash, complete_qry.hashes[i]) == 0) {
            return true;  
        }
    }
    return false; 
}

void merge_queries(struct bpkg_query* dest, struct bpkg_query* src) {
    if (dest->len + src->len >= dest->capacity) {
        while (dest->capacity <= dest->len + src->len) {
            dest->capacity *= 2;
        }
        char** new_hashes = realloc(dest->hashes, dest->capacity * sizeof(char*));
        if (!new_hashes) {
            perror("realloc failed");
            exit(EXIT_FAILURE);
        }
        dest->hashes = new_hashes;
    }

    for (size_t i = 0; i < src->len; i++) {
        dest->hashes[dest->len++] = src->hashes[i];
        src->hashes[i] = NULL; // Avoid double free
    }
    free(src->hashes);
    src->hashes = NULL;
    src->len = 0;
}

// Recursively search in left and right subtrees until the hash is found
struct merkle_tree_node* find_node_by_hash(struct merkle_tree_node* node, const char* hash) {
    if (node == NULL) {
        return NULL;  // Base case: node is NULL
    }
    if (strcmp(node->computed_hash, hash) == 0) {
        return node;  // Hash matches
    }

    struct merkle_tree_node* leftResult = find_node_by_hash(node->left, hash);
    if (leftResult != NULL) return leftResult;

    struct merkle_tree_node* rightResult = find_node_by_hash(node->right, hash);
    return rightResult;
}

void collect_leaf_hashes(struct merkle_tree_node* node, struct bpkg_query* qry, size_t* capacity) {
    if (node == NULL) {
        return; // Base case: node is NULL
    }
    if (node->left == NULL && node->right == NULL) { // It's a leaf node
        if (qry->len >= *capacity) { // Need more space
            *capacity *= 2; // Double the capacity
            qry->hashes = realloc(qry->hashes, *capacity * sizeof(char*)); // Attempt to resize
            if (qry->hashes == NULL) {
                perror("Failed to realloc memory for hashes");
                exit(EXIT_FAILURE);
            }
        }
        qry->hashes[qry->len] = strdup(node->computed_hash); // Store the hash
        qry->len++; // Increment the length
        return;
    }
    // Recursively collect hashes from both subtrees
    collect_leaf_hashes(node->left, qry, capacity);
    collect_leaf_hashes(node->right, qry, capacity);
}

/**
 * Retrieves a list of all hashes within the package/tree
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_all_hashes(struct bpkg_obj* bpkg) {
    struct bpkg_query qry = { 0 };
    size_t initial_capacity = 10;
    size_t index = 0;

    // Initialize memory for hash pointers
    qry.hashes = malloc(initial_capacity * sizeof(char*));
    if (qry.hashes == NULL) {
        perror("Memory allocation failed for qry.hashes");
        exit(EXIT_FAILURE);
    }

    // Collect hashes from the Merkle tree
    collect_hashes_inorder(bpkg->merkle_root, &qry.hashes, &index, &initial_capacity);

    // Set the actual number of hashes
    qry.len = index;

    return qry;
}


/**
 * Retrieves all completed chunks of a package object
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_completed_chunks(struct bpkg_obj* obj) { 
    struct bpkg_query qry = {0};
    FILE* file = fopen(obj->filename, "rb");
    if (!file) {
        printf("%s\n", obj->filename);
        perror("Unable to open file");
        return qry;
    }

    long chunkSize = obj->size / obj->nchunks;
    uint8_t* buffer = malloc(chunkSize);
    if (!buffer) {
        perror("Failed to allocate memory for buffer");
        fclose(file);
        return qry;
    }

    struct sha256_compute_data cdata;
    uint8_t hash[SHA256_DIGEST_LENGTH];
    char hexHash[MAX_HASH_LEN + 1] = {0}; 

    qry.hashes = malloc(obj->nchunks * sizeof(char*));
    if (qry.hashes == NULL) {
        perror("Memory allocation failed for qry.hashes");
        free(buffer);
        fclose(file);
        return qry;
    }

    qry.len = 0;

    for (int i = 0; i < obj->nchunks; i++) {
        if (fread(buffer, 1, chunkSize, file) != chunkSize) {
            perror("Failed to read full chunk");
            continue;
        }

        sha256_compute_data_init(&cdata);
        sha256_update(&cdata, buffer, chunkSize);
        sha256_finalize(&cdata, hash);
        sha256_output_hex(&cdata, hexHash);

        if (strcmp(hexHash, obj->chunks[i].hash) == 0) {
            qry.hashes[qry.len] = strdup(hexHash);
            qry.len++;
        }
    }

    free(buffer);
    fclose(file);

    // Resize array of hashes to match the number of matches found
    if (qry.len > 0) {
        qry.hashes = realloc(qry.hashes, qry.len * sizeof(char*));
    } else {
        free(qry.hashes);
        qry.hashes = NULL;
    }

    return qry;
}

/**
 * Gets only the required/min hashes to represent the current completion state
 * Return the smallest set of hashes of completed branches to represent
 * the completion state of the file.
 *
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_min_completed_hashes(struct bpkg_obj* bpkg) {
    struct bpkg_query qry = { 0 };
    struct bpkg_query chunk_qry = bpkg_get_completed_chunks(bpkg);
    qry.capacity = 10;
    qry.hashes = malloc(qry.capacity * sizeof(char*));
    if (qry.hashes == NULL) {
        perror("Failed to allocate memory for hash array");
        exit(EXIT_FAILURE);
    }
    qry.len = 0;

    if (bpkg->merkle_root) {
        collect_min_hashes(bpkg->merkle_root, &qry, &chunk_qry);
    }
    bpkg_query_destroy(&chunk_qry);
    return qry;
}


/**
 * Retrieves all chunk hashes given a certain an ancestor hash (or itself)
 * Example: If the root hash was given, all chunk hashes will be outputted
 * 	If the root's left child hash was given, all chunks corresponding to
 * 	the first half of the file will be outputted
 * 	If the root's right child hash was given, all chunks corresponding to
 * 	the second half of the file will be outputted
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_all_chunk_hashes_from_hash(struct bpkg_obj* bpkg, char* hash) {
    struct bpkg_query qry = {0};
    size_t capacity = 10; // Starting capacity
    qry.hashes = malloc(MAX_HASH_LEN); // Allocate initial array
    if (qry.hashes == NULL) {
        perror("Memory allocation failed for qry.hashes");
        exit(EXIT_FAILURE);
    }
    qry.len = 0;

    struct merkle_tree_node* foundNode = find_node_by_hash(bpkg->merkle_root, hash);
    if (foundNode == NULL) {
        printf("No node found with the given hash: %s\n", hash);
        return qry;
    }

    collect_leaf_hashes(foundNode, &qry, &capacity);

    return qry;
}


/**
 * Deallocates the query result after it has been constructed from
 * the relevant queries above.
 */
void bpkg_query_destroy(struct bpkg_query* qry) {
    if (qry->hashes != NULL) {
        for (size_t i = 0; i < qry->len; i++) {
            free(qry->hashes[i]);
        }
        free(qry->hashes); 
        qry->hashes = NULL; 
    }
}

/**
 * Deallocates memory at the end of the program,
 * make sure it has been completely deallocated
 */
// Free dynamically allocated memory before exiting the program
void bpkg_obj_destroy(struct bpkg_obj* obj) {
    if (obj->hashes != NULL) {
        for (int i = 0; i < obj->nhashes; i++) {
            free(obj->hashes[i]);
        }
        free(obj->hashes);
    }
    
    if (obj->chunks != NULL) {
        free(obj->chunks);
    }
    if (obj->merkle_root) {
        free_merkle_tree(obj->merkle_root);
    }
    
    free(obj);
}

// int main() {
//      bpkg_load("resources/pkgs/file4.bpkg");
// }


