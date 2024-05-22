#ifndef PKGCHK_H
#define PKGCHK_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<stdbool.h>

#define MAX_IDENT_LEN 1024
#define MAX_FILENAME_LEN 255
#define MAX_HASH_LEN 64

/**
 * Query object, allows you to assign
 * hash strings to it.
 * Typically: malloc N number of strings for hashes
 *    after malloc the space for each string
 *    Make sure you deallocate in the destroy function
 */
struct bpkg_query {
	char** hashes;
	size_t len;
    size_t capacity;
};

struct merkle_tree_node;

// Structure to represent a chunk within the package
typedef struct chunk_obj {
    char hash[MAX_HASH_LEN];
    uint32_t offset;
    uint32_t size;
} Chunk;

// Structure to represent the package object
typedef struct bpkg_obj {
    char ident[MAX_IDENT_LEN + 1];
    char filename[MAX_FILENAME_LEN + 1];
    uint32_t size;
    uint32_t nhashes;
    char **hashes;
    uint32_t nchunks;
    Chunk *chunks;
    struct merkle_tree_node* merkle_root;
} BpkgObj;

bool is_chunk_complete(struct merkle_tree_node* node, struct bpkg_query complete_qry);
void ensure_capacity_and_add_hash(struct bpkg_query* qry, char* hash);
void merge_if_complete(struct bpkg_query* dest, struct bpkg_query* src, struct bpkg_query* complete_chunks);
void free_query(struct bpkg_query* qry);
bool is_chunk_complete_by_hash(const char* hash, struct bpkg_query* complete_chunks);
bool all_hashes_complete(struct bpkg_query* qry, struct bpkg_query* complete_chunks);
struct bpkg_query compare_files(struct bpkg_obj* obj, const char* filepath);

void merge_queries(struct bpkg_query* dest, struct bpkg_query* src);

/**
 * Loads the package for when a value path is given
 */
struct bpkg_obj* bpkg_load(const char* path);

/**
 * Checks to see if the referenced filename in the bpkg file
 * exists or not.
 * @param bpkg, constructed bpkg object
 * @return query_result, a single string should be
 *      printable in hashes with len sized to 1.
 * 		If the file exists, hashes[0] should contain "File Exists"
 *		If the file does not exist, hashes[0] should contain "File Created"
 */
struct bpkg_query bpkg_file_check(struct bpkg_obj* bpkg);

/**
 * Retrieves a list of all hashes within the package/tree
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_all_hashes(struct bpkg_obj* bpkg);

/**
 * Retrieves all completed chunks of a package object
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_completed_chunks(struct bpkg_obj* bpkg);


/**
 * Gets the mininum of hashes to represented the current completion state
 * Example: If chunks representing start to mid have been completed but
 * 	mid to end have not been, then we will have (N_CHUNKS/2) + 1 hashes
 * 	outputted
 *
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_min_completed_hashes(struct bpkg_obj* bpkg); 


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
struct bpkg_query bpkg_get_all_chunk_hashes_from_hash(struct bpkg_obj* bpkg, char* hash);


/**
 * Deallocates the query result after it has been constructed from
 * the relevant queries above.
 */
void bpkg_query_destroy(struct bpkg_query* qry);

/**
 * Deallocates memory at the end of the program,
 * make sure it has been completely deallocated
 */
void bpkg_obj_destroy(struct bpkg_obj* obj);

#endif

