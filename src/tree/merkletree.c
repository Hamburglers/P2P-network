#include "chk/pkgchk.h"
#include "crypt/sha256.h"
#include "tree/merkletree.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// function to create the node given a hash, and whether its a leaf
Merkle_tree_node *create_merkle_tree_node(const char *hash, int is_leaf)
{
    Merkle_tree_node *node = calloc(1, sizeof(Merkle_tree_node));
    if (!node)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    node->is_leaf = is_leaf;
    if (hash[0] != '\0')
    {
        // memcpy(node->computed_hash, hash, SHA256_HEXLEN);
        // node->computed_hash[64] = '\0';
        // placeholder
        memset(node->computed_hash, 0, SHA256_HEXLEN);
        if (is_leaf)
        {
            memcpy(node->expected_hash, hash, SHA256_HEXLEN);
            node->expected_hash[64] = '\0';
        }
    }
    else
    {
        // placeholder
        memset(node->computed_hash, 0, SHA256_HEXLEN);
        memset(node->expected_hash, 0, SHA256_HEXLEN);
    }
    return node;
}

// calculate the parent hash given left, and right hash
void compute_parent_hash(Merkle_tree_node *node)
{
    if (node && node->left && node->right)
    {
        char concat_hash[2 * SHA256_HEXLEN + 1] = {0};
        snprintf(concat_hash, sizeof(concat_hash), "%s%s", 
        node->left->computed_hash, node->right->computed_hash);
        // printf("Concatenated Hash Input: %s\n", concat_hash);

        struct sha256_compute_data cdata;
        sha256_compute_data_init(&cdata);
        sha256_update(&cdata, (uint8_t *)concat_hash, strlen(concat_hash));
        uint8_t hashout[32];
        sha256_finalize(&cdata, hashout);
        sha256_output_hex(&cdata, node->computed_hash);
    }
}

// level order traversal
// wrapper to have a next attribute
typedef struct NodeQueue
{
    Merkle_tree_node *node;
    struct NodeQueue *next;
} NodeQueue;

// enqueue method
void enqueue(NodeQueue **head, NodeQueue **tail, Merkle_tree_node *node)
{
    NodeQueue *new_node = (NodeQueue *)malloc(sizeof(NodeQueue));
    new_node->node = node;
    new_node->next = NULL;
    if (*tail != NULL)
    {
        (*tail)->next = new_node;
    }
    *tail = new_node;
    if (*head == NULL)
    {
        *head = new_node;
    }
}

// dequeue method returns new pointer to head
Merkle_tree_node *dequeue(NodeQueue **head, NodeQueue **tail)
{
    if (*head == NULL)
        return NULL;
    NodeQueue *temp = *head;
    Merkle_tree_node *result = temp->node;
    *head = temp->next;
    if (*head == NULL)
    {
        *tail = NULL;
    }
    free(temp);
    return result;
}

Merkle_tree *intialise_merkle_tree(bpkg_obj *obj)
{
    // check if obj has valid parameters
    if (!obj || obj->nchunks == 0) {
        fprintf(stderr, "Invalid bpkg object parameter: nchunks\n");
        return NULL;
    }

    Merkle_tree *tree = calloc(1, sizeof(Merkle_tree));
    if (!tree) {
        fprintf(stderr, "Error allocating memory\n");
        return NULL;
    }

    Merkle_tree_node **nodes = calloc(obj->nchunks, sizeof(Merkle_tree_node *));
    if (!nodes)
    {
        fprintf(stderr, "Error allocating memory\n");
        free(tree);
        return NULL;
    }

    size_t total_nodes = 0;
    uint8_t *buffer;
    // Initialize leaf nodes
    FILE *file = fopen(obj->filename, "rb");
    if (!file)
    {
        fprintf(stderr, "Error opening file\n");
        free(tree);
        free(nodes);
        return NULL;
    }
    // create the leaf nodes
    for (size_t i = 0; i < obj->nchunks; i++)
    {
        Chunk specific = obj->chunks[i];
        nodes[i] = create_merkle_tree_node(specific.hash, 1);
        // if failed, free everything before
        if (!nodes[i])
        {
            fprintf(stderr, "Error allocating merkle tree node\n");
            while (i > 0)
                free(nodes[--i]);
            free(nodes);
            free(tree);
            fclose(file);
            return NULL;
        }
        buffer = malloc(obj->chunks[i].size);
        // if failed, free everything before
        if (!buffer)
        {
            while (i > 0)
                free(nodes[--i]);
            free(nodes);
            free(tree);
            fclose(file);
            fprintf(stderr, "Error allocating memory\n");
            return NULL;
        }

        fseek(file, obj->chunks[i].offset, SEEK_SET);
        // if data was not read correctly
        if (fread(buffer, 1, obj->chunks[i].size, file) != obj->chunks[i].size)
        {
            fprintf(stderr, "Error reading from .dat file\n");
            free(buffer);
            while (i > 0)
                free(nodes[--i]);
            free(nodes);
            free(tree);
            fclose(file);
            return NULL;
        }
        // read from the offset and compute the hash
        struct sha256_compute_data cdata;
        sha256_compute_data_init(&cdata);
        sha256_update(&cdata, buffer, obj->chunks[i].size);
        uint8_t hashout[32];
        sha256_finalize(&cdata, hashout);
        sha256_output_hex(&cdata, nodes[i]->computed_hash);
        nodes[i]->computed_hash[64] = '\0';
        free(buffer);
        total_nodes++;
    }
    fclose(file);

    // build tree from down up
    size_t current_level_count = obj->nchunks;
    while (current_level_count > 1)
    {
        size_t next_level_count = 0;
        for (size_t i = 0; i < current_level_count; i += 2)
        {
            // for every 2 pair, create a new parent and replace value in array
            if (i + 1 < current_level_count)
            {
                Merkle_tree_node *parent = create_merkle_tree_node("", 0);
                if (!parent) {
                    fprintf(stderr, "Error creating merkle tree node\n");
                    free(nodes);
                    free(tree);
                    return NULL;
                }
                parent->left = nodes[i];
                parent->right = nodes[i + 1];
                compute_parent_hash(parent);
                nodes[next_level_count++] = parent;
            }
            else
            {
                // odd case
                nodes[next_level_count++] = nodes[i];
            }
            total_nodes++;
        }
        // move up
        current_level_count = next_level_count;
    }
    // set root and number of nodes
    tree->root = nodes[0];
    tree->n_nodes = total_nodes;
    // free pointer array
    free(nodes);

    NodeQueue *head = NULL, *tail = NULL;
    enqueue(&head, &tail, tree->root);
    int i = 0;
    while (head != NULL)
    {
        Merkle_tree_node *current = dequeue(&head, &tail);
        // update level order hashes unless index exceeded then its the base
        if (i < obj->nhashes)
        {
            memcpy(current->expected_hash, obj->hashes[i], SHA256_HEXLEN);
            current->expected_hash[64] = '\0';
        }
        if (current->left != NULL)
        {
            enqueue(&head, &tail, current->left);
        }
        if (current->right != NULL)
        {
            enqueue(&head, &tail, current->right);
        }
        i++;
    }
    return tree;
}

// to free the entire tree using dfs
void destroy_merkle_tree(Merkle_tree_node *node)
{
    if (node)
    {
        destroy_merkle_tree(node->left);
        destroy_merkle_tree(node->right);
        free(node);
    }
}

// for debugging, print out the entire tree
void debug(Merkle_tree *tree)
{
    if (tree->root == NULL)
        return;

    NodeQueue *head = NULL, *tail = NULL;
    enqueue(&head, &tail, tree->root);

    while (head != NULL)
    {
        Merkle_tree_node *current = dequeue(&head, &tail);

        printf("Expected hash: %s Computed hash: %s\n",
               current->expected_hash, current->expected_hash);

        // Enqueue children if they exist
        if (current->left != NULL)
        {
            enqueue(&head, &tail, current->left);
        }
        if (current->right != NULL)
        {
            enqueue(&head, &tail, current->right);
        }
    }
}

// for bpkg get all hashes very similar to one used inside intialise
char **levelOrderTraversal(bpkg_obj *bpkg)
{
    if (bpkg->merkle->root == NULL)
        return NULL;

    char **hashes = malloc(bpkg->merkle->n_nodes * sizeof(char *));
    if (hashes == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    NodeQueue *head = NULL, *tail = NULL;
    enqueue(&head, &tail, bpkg->merkle->root);

    size_t hash_count = 0;

    while (head != NULL && hash_count < bpkg->merkle->n_nodes)
    {
        Merkle_tree_node *current = dequeue(&head, &tail);

        // Allocate memory for the hash string
        // printf("%ld\n", hash_count);
        hashes[hash_count] = malloc((SHA256_HEXLEN + 1) * sizeof(char));
        if (hashes[hash_count] == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            for (size_t i = 0; i < hash_count; ++i)
                free(hashes[i]);
            free(hashes);
            return NULL;
        }
        // printf("%s\n", current->expected_hash);
        memcpy(hashes[hash_count], current->expected_hash, SHA256_HEXLEN);
        hashes[hash_count][SHA256_HEXLEN] = '\0';
        ++hash_count;

        if (current->left != NULL)
        {
            enqueue(&head, &tail, current->left);
        }
        if (current->right != NULL)
        {
            enqueue(&head, &tail, current->right);
        }
        // printf("---------\n");
        // printf("%s\n", current->expected_hash);
        // printf("%s\n", current->left->expected_hash);
        // printf("%s\n", current->right->expected_hash);
        // printf("---------\n");
    }
    return hashes;
}

// preorder traversal for function 1 of get_complete_chunks
void collect_matching_leaf_hashes(Merkle_tree_node *node, 
char ***hashes, size_t *len)
{
    if (node == NULL)
        return;

    if (node->is_leaf)
    {
        // printf("%s\n", node->expected_hash);
        // printf("%s\n", node->computed_hash);
        if (strcmp(node->expected_hash, node->computed_hash) == 0)
        {
            (*hashes)[*len] = malloc((SHA256_HEXLEN + 1) * sizeof(char));
            if ((*hashes)[*len] != NULL)
            {
                strcpy((*hashes)[*len], node->computed_hash);
                (*len)++;
            }
        }
    }
    else
    {
        collect_matching_leaf_hashes(node->left, hashes, len);
        collect_matching_leaf_hashes(node->right, hashes, len);
    }
}

// preorder traversal for function 2 of get_complete_chunks
void collect_min_hashes(Merkle_tree_node *node, char ***hashes, size_t *len)
{
    if (node == NULL)
        return;
    
    if (strcmp(node->expected_hash, node->computed_hash) == 0)
    {
        (*hashes)[*len] = malloc((SHA256_HEXLEN + 1) * sizeof(char));
        if ((*hashes)[*len] != NULL)
        {
            strcpy((*hashes)[*len], node->expected_hash);
            (*len)++;
        }
        return;
    } else {
        collect_min_hashes(node->left, hashes, len);
        collect_min_hashes(node->right, hashes, len);
    }
}

// preorder traversal for function 3 of get_complete_chunks
void collect_leaf_hashes(Merkle_tree_node *node, char ***hashes, size_t *len)
{
    if (node == NULL)
        return;
    if (node->is_leaf)
    {
        (*hashes)[*len] = malloc((SHA256_HEXLEN + 1) * sizeof(char));
        if ((*hashes)[*len] != NULL)
        {
            strcpy((*hashes)[*len], node->expected_hash);
            (*len)++;
        }
    }
    else
    {
        collect_leaf_hashes(node->left, hashes, len);
        collect_leaf_hashes(node->right, hashes, len);
    }
}

// preorder traversal for function 3 of get_complete_chunks
void collect_matching_chunk_hash(Merkle_tree_node *node, 
char ***hashes, size_t *len, char *hash)
{
    if (node == NULL)
        return;
    // hash is not null terminated
    // printf("---\n%.64s\n", node->computed_hash);
    // printf("%.64s\n---\n", hash);
    if (strncmp(node->expected_hash, hash, 64) == 0)
    {
        // if its the leaf then there are no children left, add hash and return
        if (node->is_leaf) {
            (*hashes)[*len] = malloc((SHA256_HEXLEN + 1) * sizeof(char));
            if ((*hashes)[*len] != NULL)
            {
                strcpy((*hashes)[*len], node->expected_hash);
                (*len)++;
            }
            return;
        }
        collect_leaf_hashes(node->left, hashes, len);
        collect_leaf_hashes(node->right, hashes, len);
    } else {
        collect_matching_chunk_hash(node->left, hashes, len, hash);
        collect_matching_chunk_hash(node->right, hashes, len, hash);
    }
}

// function to get completed chunks which have same expected + computed hash
bpkg_query get_complete_chunks(bpkg_obj *obj, int flag, char* hash)
{
    bpkg_query qy = {0};
    if (obj->merkle == NULL || obj->merkle->root == NULL) {
        fprintf(stderr, "Error with bpkg_obj merkle tree\n");
        qy.hashes = NULL;
        return qy;
    }

    // at most nchunks of hashes will be needed
    char **hashes = malloc(obj->nchunks * sizeof(char *));
    if (hashes == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        qy.hashes = NULL;
        return qy;
    }

    size_t len = 0;
    // bpkg_get_completed_chunks
    if (flag == 0) {
        collect_matching_leaf_hashes(obj->merkle->root, &hashes, &len);
    // bpkg_get_min_completed_hashes
    } else if (flag == 1) {
        collect_min_hashes(obj->merkle->root, &hashes, &len);
    // bpkg_get_all_chunk_hashes_from_hash
    } else if (flag == 2) {
        collect_matching_chunk_hash(obj->merkle->root, &hashes, &len, hash);
    } else {
        // should never happen
        fprintf(stderr, "Invalid flag provided to get_complete_chunks\n");
        free(hashes);
        qy.hashes = NULL;
        return qy;
    }
    if (len == 0)
    {
        free(hashes);
        qy.hashes = NULL;
        fprintf(stderr, "No hashes collected\n");
    }
    else
    {
        qy.hashes = hashes;
        qy.len = len;
    }
    return qy;
}
