#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "chk/pkgchk.h"
#include "tree/merkletree.h"
// PART 1

#define HASHLENGTH 65
#define MAXLINELENGTH 2000
/**
 * Loads the package for when a valid path is given
 */
bpkg_obj *bpkg_load(const char *path)
{
    // open in read mode
    FILE *file = fopen(path, "r");
    // failed to open file
    if (!file)
    {
        fprintf(stderr, "Error opening file\n");
        return NULL;
    }
    // allocate memory for struct obj
    bpkg_obj *obj = calloc(1, sizeof(bpkg_obj));
    // alloc failed
    if (!obj)
    {
        fprintf(stderr, "Error allocating memory\n");
        fclose(file);
        return NULL;
    }
    // struct to make sure every field has been parsed
    ParseFlags flags = {0};
    // create buffer with max size
    char buf[MAXLINELENGTH];
    // keep reading from file
    while (fgets(buf, sizeof(buf), file) != NULL)
    {
        buf[strcspn(buf, "\n")] = 0;
        // ident
        if (strncmp(buf, "ident:", 6) == 0)
        {
            strncpy(obj->ident, buf + 6, sizeof(obj->ident) - 1);
            if (flags.ident == 0)
            {
                flags.ident = 1;
            }
            else
            {
                fprintf(stderr, "Repeat field found in bpkg - ident:\n");
                bpkg_obj_destroy(obj);
                return NULL;
            }
        }
        // filename
        else if (strncmp(buf, "filename:", 9) == 0)
        {
            strncpy(obj->filename, buf + 9, sizeof(obj->filename) - 1);
            if (flags.filename == 0)
            {
                flags.filename = 1;
            }
            else
            {
                fprintf(stderr, "Repeat field found in bpkg - filename:\n");
                bpkg_obj_destroy(obj);
                return NULL;
            }
        }
        // size
        else if (strncmp(buf, "size:", 5) == 0)
        {
            obj->size = atoi(buf + 5);
            if (obj->size <= 0)
            {
                fprintf(stderr, "Invalid size found\n");
                bpkg_obj_destroy(obj);
                return NULL;
            }
            if (flags.size == 0)
            {
                flags.size = 1;
            }
            else
            {
                fprintf(stderr, "Repeat field found in bpkg - size:\n");
                bpkg_obj_destroy(obj);
                return NULL;
            }
        }
        // nhashes
        else if (strncmp(buf, "nhashes:", 8) == 0)
        {
            obj->nhashes = atoi(buf + 8);
            if (obj->nhashes <= 0)
            {
                fprintf(stderr, "Invalid number for nhashes\n");
                bpkg_obj_destroy(obj);
                return NULL;
            }
            if (flags.nhashes == 0)
            {
                flags.nhashes = 1;
            }
            else
            {
                fprintf(stderr, "Repeat field found in bpkg - nhashes:\n");
                bpkg_obj_destroy(obj);
                return NULL;
            }
        }
        // hashes
        else if (strncmp(buf, "hashes:", 7) == 0)
        {
            // if not intialised
            if (obj->nhashes <= 0)
            {
                fprintf(stderr, "Attempted to store hashes before "
                    "nhashes was given\n");
                bpkg_obj_destroy(obj);
                fclose(file);
                return NULL;
            }
            if (flags.hashes == 0)
            {
                flags.hashes = 1;
            }
            else
            {
                fprintf(stderr, "Repeat field found in bpkg - hashes:\n");
                bpkg_obj_destroy(obj);
                return NULL;
            }
            // set up memory to store char*
            obj->hashes = calloc(obj->nhashes, sizeof(char *));
            // failed memory allocation
            if (!obj->hashes)
            {
                fclose(file);
                bpkg_obj_destroy(obj);
                fprintf(stderr, "Memory allocation failed\n");
                return NULL;
            }
            // get all n hashes
            for (uint32_t i = 0; i < obj->nhashes; i++)
            {
                // allocate memory for string
                obj->hashes[i] = malloc(HASHLENGTH);
                // failed memory allocation or file not read correctly
                if (!obj->hashes[i] || 
                fscanf(file, "%64s", obj->hashes[i]) != 1 || 
                strlen(obj->hashes[i]) != 64)
                {
                    fclose(file);
                    // if its allocated so if its not length 64
                    if (obj->hashes[i])
                    {
                        free(obj->hashes[i]);
                    }
                    // cleanup and free all
                    for (uint32_t j = 0; j < i; j++)
                    {
                        free(obj->hashes[j]);
                    }
                    free(obj->hashes);
                    // set to null so bpkgobj doesnt try free again
                    obj->hashes = NULL;
                    bpkg_obj_destroy(obj);
                    fprintf(stderr, "File parsing error or "
                    "memory allocation failed\n");
                    return NULL;
                }
            }
        }
        // nchunks
        else if (strncmp(buf, "nchunks:", 8) == 0)
        {
            obj->nchunks = atoi(buf + 8);
            if (obj->nchunks <= 0)
            {
                fprintf(stderr, "Invalid number for nchunks\n");
                bpkg_obj_destroy(obj);
                return NULL;
            }
            if (flags.nchunks == 0)
            {
                flags.nchunks = 1;
            }
            else
            {
                fprintf(stderr, "Repeat field found in bpkg - nchunks:\n");
                bpkg_obj_destroy(obj);
                return NULL;
            }
        }
        // chunks
        else if (strncmp(buf, "chunks:", 7) == 0)
        {
            // if not intialised
            if (obj->nchunks <= 0)
            {
                fprintf(stderr, "Attempted to store chunks before "
                "nchunks was given\n");
                fclose(file);
                bpkg_obj_destroy(obj);
                return NULL;
            }
            if (flags.chunks == 0)
            {
                flags.chunks = 1;
            }
            else
            {
                fprintf(stderr, "Repeat field found in bpkg - chunks:\n");
                bpkg_obj_destroy(obj);
                return NULL;
            }
            // allocate memory for structs
            obj->chunks = calloc(obj->nchunks, sizeof(Chunk));
            // failed memory allocation
            if (!obj->chunks)
            {
                fclose(file);
                bpkg_obj_destroy(obj);
                fprintf(stderr, "Memory allocation failed\n");
                return NULL;
            }
            // get all n chunks
            for (uint32_t i = 0; i < obj->nchunks; i++)
            {
                // if file not read correctly
                if (fscanf(file, "%64s,%u,%u", obj->chunks[i].hash, 
                &obj->chunks[i].offset, &obj->chunks[i].size) != 3 || 
                strlen(obj->chunks[i].hash) != 64 || 
                obj->chunks[i].offset < 0 || 
                obj->chunks[i].size <= 0)
                {
                    // cleanup
                    fclose(file);
                    bpkg_obj_destroy(obj);
                    fprintf(stderr, "File parsing error\n");
                    return NULL;
                }
            }
        }
    }
    fclose(file);
    // check all flags have been parsed
    if (!flags.ident || !flags.filename || !flags.size || !flags.nhashes 
    || !flags.hashes || !flags.nchunks || !flags.chunks)
    {
        fprintf(stderr, "Missing required fields in the package data\n");
        bpkg_obj_destroy(obj);
        return NULL;
    }
    return obj;
}

int bpkg_intialise_merkle(bpkg_obj *obj)
{
    // intialise the merkle tree
    obj->merkle = intialise_merkle_tree(obj);
    // if something went wrong
    if (obj->merkle == NULL)
    {
        fprintf(stderr, "Error intialising merkle tree\n");
        bpkg_obj_destroy(obj);
        return 1;
    }
    return 0;
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
bpkg_query bpkg_file_check(bpkg_obj *bpkg)
{
    // debug(bpkg->merkle);
    bpkg_query result = {0};
    // 2d list for list of strings
    result.hashes = malloc(sizeof(char *));
    // memory allocation failed
    if (!result.hashes)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    // enough for both strings
    result.hashes[0] = malloc(13);
    // memory allocation failed
    if (!result.hashes[0])
    {
        free(result.hashes);
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    // test if can open
    FILE *file = fopen(bpkg->filename, "r");
    // opened succesfully
    if (file)
    {
        fclose(file);
        strcpy(result.hashes[0], "File Exists");
        // could not be opened then create the file with the correct size
    }
    else
    {
        // if somehow cannot write
        file = fopen(bpkg->filename, "w");
        if (!file)
        {
            free(result.hashes[0]);
            free(result.hashes);
            fprintf(stderr, "Failed to create file\n");
            exit(EXIT_FAILURE);
        }
        // move pointer to resize file size
        if (fseek(file, bpkg->size - 1, SEEK_SET) != 0 || 
        fwrite("\0", 1, 1, file) != 1)
        {
            fclose(file);
            free(result.hashes[0]);
            free(result.hashes);
            fprintf(stderr, "Failed to set file size\n");
            exit(EXIT_FAILURE);
        }
        fclose(file);
        strcpy(result.hashes[0], "File Created");
    }
    result.len = 1;
    return result;
}

// for debugging
void print_bpkg_obj(const bpkg_obj *obj, const char *output_path)
{
    FILE *fp = fopen(output_path, "w");
    if (!fp)
    {
        fprintf(stderr, "Failed to open output file\n");
        return;
    }

    fprintf(fp, "ident:%s\n", obj->ident);
    fprintf(fp, "filename:%s\n", obj->filename);
    fprintf(fp, "size:%u\n", obj->size);
    fprintf(fp, "nhashes:%u\n", obj->nhashes);

    if (obj->nhashes > 0)
    {
        fprintf(fp, "hashes:\n");
        for (uint32_t i = 0; i < obj->nhashes; i++)
        {
            fprintf(fp, "\t%s\n", obj->hashes[i]);
        }
    }

    fprintf(fp, "nchunks:%u\n", obj->nchunks);
    if (obj->nchunks > 0)
    {
        fprintf(fp, "chunks:\n");
        for (uint32_t i = 0; i < obj->nchunks; i++)
        {
            fprintf(fp, "\t%s,%u,%u\n", obj->chunks[i].hash, 
            obj->chunks[i].offset, obj->chunks[i].size);
        }
    }

    fclose(fp);
}

/**
 * Retrieves a list of all hashes within the package/tree
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
bpkg_query bpkg_get_all_hashes(bpkg_obj *bpkg)
{
    bpkg_query query;
    // debug(bpkg->merkle);
    query.hashes = levelOrderTraversal(bpkg);
    if (!query.hashes)
    {
        fprintf(stderr, "Error with bpkg_obj fields\n");
        exit(EXIT_FAILURE);
    }
    query.len = bpkg->merkle->n_nodes;
    return query;
}

/**
 * Retrieves all completed chunks of a package object
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
bpkg_query bpkg_get_completed_chunks(bpkg_obj *bpkg)
{
    // flag of 1 since bpkg_get_min_completed_hashes is very similar
    // this calls a different function inside get_complete_chunks
    bpkg_query qy = get_complete_chunks(bpkg, 0, "");
    if (qy.hashes == NULL)
    {
        fprintf(stderr, "Function get_complete_chunks failed\n");
        exit(EXIT_FAILURE);
    }
    return qy;
}

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
bpkg_query bpkg_get_min_completed_hashes(bpkg_obj *bpkg)
{
    // flag of 0 since bpkg_get_min_completed_hashes is very similar
    // this calls a different function inside get_complete_chunks
    bpkg_query qy = get_complete_chunks(bpkg, 1, "");
    if (qy.hashes == NULL)
    {
        fprintf(stderr, "Function get_complete_chunks failed\n");
        exit(EXIT_FAILURE);
    }
    return qy;
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
bpkg_query bpkg_get_all_chunk_hashes_from_hash(bpkg_obj *bpkg, char *hash)
{
    // flag of 2 since bpkg_get_min_completed_hashes is very similar
    // this calls a different function inside get_complete_chunks
    bpkg_query qy = get_complete_chunks(bpkg, 2, hash);
    if (qy.hashes == NULL)
    {
        fprintf(stderr, "Function get_complete_chunks failed\n");
        exit(EXIT_FAILURE);
    }
    return qy;
}

/**
 * Deallocates the query result after it has been constructed from
 * the relevant queries above.
 */
void bpkg_query_destroy(bpkg_query *qry)
{
    if (qry->hashes)
    {
        for (size_t i = 0; i < qry->len; i++)
        {
            free(qry->hashes[i]);
        }
        free(qry->hashes);
    }
}

/**
 * Deallocates memory at the end of the program,
 * make sure it has been completely deallocated
 */
void bpkg_obj_destroy(bpkg_obj *obj)
{
    if (obj)
    {
        if (obj->merkle)
        {
            destroy_merkle_tree(obj->merkle->root);
            free(obj->merkle);
        }
        if (obj->hashes)
        {
            for (uint32_t i = 0; i < obj->nhashes; i++)
            {
                if (obj->hashes[i])
                    free(obj->hashes[i]);
            }
            free(obj->hashes);
        }
        if (obj->chunks)
        {
            free(obj->chunks);
        }
        free(obj);
    }
}
