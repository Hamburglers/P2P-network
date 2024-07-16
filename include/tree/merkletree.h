#ifndef MERKLE_TREE_H
#define MERKLE_TREE_H

#include <stddef.h>
// forward declaration due to circular dependency
typedef struct bpkg_obj bpkg_obj;
typedef struct bpkg_query bpkg_query;
#define SHA256_HEXLEN (64)

typedef struct Merkle_tree_node
{
    struct Merkle_tree_node *left;
    struct Merkle_tree_node *right;
    int is_leaf;
    char expected_hash[SHA256_HEXLEN + 1];
    char computed_hash[SHA256_HEXLEN + 1];
} Merkle_tree_node;

typedef struct
{
    Merkle_tree_node *root;
    size_t n_nodes;
} Merkle_tree;

Merkle_tree_node *create_merkle_tree_node(const char *hash, int is_leaf);

void compute_parent_hash(Merkle_tree_node *node);

Merkle_tree *intialise_merkle_tree(bpkg_obj *obj);

void destroy_merkle_tree(Merkle_tree_node *node);

char **levelOrderTraversal(bpkg_obj *bpkg);

bpkg_query get_complete_chunks(bpkg_obj *obj, int flag, char *hash);
#endif
