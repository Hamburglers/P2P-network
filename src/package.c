#include "chk/pkgchk.h"
#include "bytetide/btide.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXIDENTLENGTH 1024
#define MAXFILESIZE 256
#define MAXHASHLENGTH 65

// handle ADDPACKAGE command
void handle_add_package(char command[], int *current_length, 
int *max_size, bpkg_obj ***list, char directory[])
{
    char filename[MAXLINELENGTH];
    // check if command doesnt have trailing spaces
    char remainder;
    if (command[10] != ' ' || command[11] == ' ' || command[11] == '\0')
    {
        printf("Invalid Input\n");
        return;
    }
    // check if there is a file specified
    if (sscanf(command + 11, "%s%c", filename, &remainder) < 1 
        || (remainder != '\n' && remainder != '\r'))
    {
        printf("Missing file argument\n");
        return;
    }

    FILE *file = fopen(filename, "r");
    // failed to open file
    if (!file)
    {
        printf("Cannot open file\n");
        return;
    }
    fclose(file);

    // if opened, try to load the bpkg object
    bpkg_obj *obj = bpkg_load(filename);
    if (obj == NULL)
    {
        printf("Unable to parse bpkg file\n");
        return;
    }
    // prepend the directory into filename
    int size = strlen(directory);
    int need_slash = directory[size - 1] != '/';
    int used_size = strlen(obj->filename);
    // check total used = size of directory 
    // + current file size + null byte + slash
    int total = size + used_size + 1 + (need_slash ? 1 : 0);
    if (total > MAXFILESIZE)
    {
        fprintf(stderr, "Filename buffer is too small to hold the "
            "full path.\n");
        bpkg_obj_destroy(obj);
        return;
    }
    char new_path[MAXFILESIZE];
    snprintf(new_path, sizeof(new_path), "%s%s%s", directory, 
        need_slash ? "/" : "", obj->filename);
    // copy new path into bpkg obj
    strncpy(obj->filename, new_path, MAXFILESIZE);
    obj->filename[MAXFILESIZE - 1] = '\0';
    // try to open the file specified in bpkg
    FILE *file2 = fopen(obj->filename, "r");
    if (!file2)
    {
        // create the file with correct size if doesn't exist
        file2 = fopen(obj->filename, "w");
        if (!file2)
        {
            fprintf(stderr, "Failed to create file\n");
            bpkg_obj_destroy(obj);
            return;
        }
        if (fseek(file2, obj->size - 1, SEEK_SET) != 0 
            || fwrite("\0", 1, 1, file2) != 1)
        {
            fclose(file2);
            bpkg_obj_destroy(obj);
            return;
        }
    }
    fclose(file2);
    // intialise the merkle tree
    if (bpkg_intialise_merkle(obj))
    {
        printf("Unable to parse bpkg file\n");
        return;
    }
    // if list of current bpkg objs runs out of space, dynamically reallocate
    if (*current_length == *max_size)
    {
        bpkg_obj **new_list = realloc(*list, 
            (*max_size * 2) * sizeof(bpkg_obj *));
        if (!new_list)
        {
            perror("Realloc failed");
            bpkg_obj_destroy(obj);
            return;
        }
        *max_size *= 2;
        *list = new_list;
    }
    (*list)[(*current_length)++] = obj;
}

// function to handle REMPACKAGE command
void handle_remove_package(char command[], int *current_length, 
int *max_size, bpkg_obj **list)
{
    // make sure no trailing spaces
    if (command[10] != ' ' || command[11] == ' ' || command[11] == '\0')
    {
        printf("Invalid Input\n");
        return;
    }
    char remainder;
    char ident[MAXIDENTLENGTH];
    // make sure at least 20 characters of IDENT is specified
    if (sscanf(command + 11, "%s%c", ident, &remainder) < 1 
        || strlen(ident) < 20 || (remainder != '\n' && remainder != '\r'))
    {
        printf("Missing identifier argument, please specify" 
            " whole 1024 character or at least 20 characters\n");
        return;
    }
    int found = 0;
    // look for the same IDENT in list of packages
    for (int i = 0; i < *current_length; i++)
    {
        if (strncmp(list[i]->ident, ident, strlen(ident)) == 0)
        {
            bpkg_obj_destroy(list[i]);
            list[i] = list[--(*current_length)];
            found = 1;
            break;
        }
    }
    if (!found)
    {
        printf("Identifier provided does not match managed packages\n");
    }
    else
    {
        printf("Package has been removed\n");
    }
    return;
}

// function to handle PACKAGES command
void handle_view_package(char command[], int *current_length,
    int *max_size, bpkg_obj **list)
{
    // check for invalid input
    if (command[9] != '\0')
    {
        printf("Invalid Input\n");
        return;
    }
    if (*current_length == 0)
    {
        printf("No packages managed\n");
        return;
    }
    // print either complete or incomplete depending on root hash 
    // computed hash and expected hash matching or not
    for (int i = 0; i < *current_length; i++)
    {
        char *complete = "INCOMPLETE";
        if (strncmp(list[i]->merkle->root->expected_hash, 
            list[i]->merkle->root->computed_hash, 64) == 0)
        {
            complete = "COMPLETED";
        }
        printf("%d. %.32s, %s : %s\n", i + 1, 
            list[i]->ident, list[i]->filename, complete);
    }
    return;
}

// function to return a bpkg obj given a IDENT
bpkg_obj *check_ident(char ident[], int current_length, bpkg_obj **list)
{
    for (int i = 0; i < current_length; i++)
    {
        bpkg_obj *obj = list[i];
        if (strncmp(obj->ident, ident, MAXIDENTLENGTH) == 0)
        {
            return obj;
        }
    }
    return NULL;
}

// function to return the specified CHUNK given a bpjg obj and chunk hash
Chunk *request_hash(char hash[], bpkg_obj *obj)
{
    for (int i = 0; i < obj->nchunks; i++)
    {
        Chunk chunk = obj->chunks[i];
        if (strncmp(chunk.hash, hash, MAXHASHLENGTH - 1) == 0)
        {
            return &obj->chunks[i];
        }
    }
    return NULL;
}