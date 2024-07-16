#ifndef PKGCHK_H
#define PKGCHK_H
#define HASHLENGTH 65
#include <stddef.h>
#include <stdint.h>
#include <tree/merkletree.h>

/**
 * Query object, allows you to assign
 * hash strings to it.
 * Typically: malloc N number of strings for hashes
 *    after malloc the space for each string
 *    Make sure you deallocate in the destroy function
 */
typedef struct bpkg_query
{
	char **hashes;
	size_t len;
} bpkg_query;

typedef struct
{
	char hash[65];
	uint32_t offset;
	uint32_t size;
} Chunk;

typedef struct bpkg_obj
{
	// max 1024 chars + null
	char ident[1024];
	// max 256 chars + null
	char filename[256];
	uint32_t size;
	uint32_t nhashes;
	char **hashes;
	uint32_t nchunks;
	Chunk *chunks;
	Merkle_tree *merkle;
} bpkg_obj;

typedef struct
{
	int ident;
	int filename;
	int size;
	int nhashes;
	int hashes;
	int nchunks;
	int chunks;
} ParseFlags;

/**
 * Loads the package for when a value path is given
 */
bpkg_obj *bpkg_load(const char *path);

// intialises the merkle tree
int bpkg_intialise_merkle(bpkg_obj *obj);

/**
 * Checks to see if the referenced filename in the bpkg file
 * exists or not.
 * @param bpkg, constructed bpkg object
 * @return query_result, a single string should be
 *      printable in hashes with len sized to 1.
 * 		If the file exists, hashes[0] should contain "File Exists"
 *		If the file does not exist, hashes[0] should contain "File Created"
 */
bpkg_query bpkg_file_check(bpkg_obj *bpkg);

/**
 * Retrieves a list of all hashes within the package/tree
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
bpkg_query bpkg_get_all_hashes(bpkg_obj *bpkg);

/**
 * Retrieves all completed chunks of a package object
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
bpkg_query bpkg_get_completed_chunks(bpkg_obj *bpkg);

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
bpkg_query bpkg_get_min_completed_hashes(bpkg_obj *bpkg);

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
bpkg_query bpkg_get_all_chunk_hashes_from_hash(bpkg_obj *bpkg, char *hash);

/**
 * Deallocates the query result after it has been constructed from
 * the relevant queries above.
 */
void bpkg_query_destroy(bpkg_query *qry);

/**
 * Deallocates memory at the end of the program,
 * make sure it has been completely deallocated
 */
void bpkg_obj_destroy(bpkg_obj *obj);

#endif