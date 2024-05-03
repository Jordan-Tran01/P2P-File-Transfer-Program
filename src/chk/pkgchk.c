#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <ctype.h>
#include "include/chk/pkgchk.h"

// PART 1

/**
 * Loads the package for when a valid path is given
 */
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

            // size_t ident_len = MAX_IDENT_LEN;
            // if (ident_len > 0 && ident_start[ident_len - 1] == '\n') {
            //     ident_start[ident_len - 1] = '\0';
            //     ident_len--;
            // }

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
                obj->hashes[i] = malloc(MAX_HASH_LEN * sizeof(char));

                fgets(line, sizeof(line), bpkg_file);
                // Split the line into substrings using strtok
                char *token = strtok(line, " ");
                if (token != NULL) {
                    // Skip leading whitespace
                    while (isspace(*token)) {
                        token++;
                    }
                    strncpy(obj->hashes[i], token, MAX_HASH_LEN);
                }
            }
        } else if (strncmp(line, "chunks:", 7) == 0) {
            obj->chunks = malloc(obj->nchunks * sizeof(struct chunk_obj *));
            if (obj->chunks == NULL) {
                // Handle memory allocation failure
            }

            for (int i = 0; i < obj->nchunks; i++) {
                struct chunk_obj* newChunk = malloc(sizeof(struct chunk_obj));

                fgets(line, sizeof(line), bpkg_file);

                // Split the line into substrings using strtok
                char *token = strtok(line, ",");
                if (token != NULL) {
                    while (isspace(*token)) {
                        token++;
                    }
                    strcpy(newChunk->hash, token);
                    token = strtok(NULL, ",");
                    if (token != NULL) {
                        newChunk->offset = atoi(token);
                        token = strtok(NULL, ",");
                        if (token != NULL) {
                            newChunk->size = atoi(token);
                        }
                    }
                }

                obj->chunks[i] = *newChunk;
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
struct bpkg_query bpkg_file_check(struct bpkg_obj* bpkg);

/**
 * Retrieves a list of all hashes within the package/tree
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_all_hashes(struct bpkg_obj* bpkg) {
    struct bpkg_query qry = { 0 };
    
    return qry;
}

/**
 * Retrieves all completed chunks of a package object
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_completed_chunks(struct bpkg_obj* bpkg) { 
    struct bpkg_query qry = { 0 };
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
struct bpkg_query bpkg_get_all_chunk_hashes_from_hash(struct bpkg_obj* bpkg, 
    char* hash) {
    
    struct bpkg_query qry = { 0 };
    return qry;
}


/**
 * Deallocates the query result after it has been constructed from
 * the relevant queries above.
 */
void bpkg_query_destroy(struct bpkg_query* qry) {
    //TODO: Deallocate here!

}

/**
 * Deallocates memory at the end of the program,
 * make sure it has been completely deallocated
 */
void bpkg_obj_destroy(struct bpkg_obj* obj) {
    //TODO: Deallocate here!

}

int main() {
    bpkg_load("resources/pkgs/file1.bpkg");
}


