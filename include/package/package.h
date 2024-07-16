#include <chk/pkgchk.h>

void handle_add_package(char command[], int *current_length, 
    int *max_size, bpkg_obj ***list, char directory[]);

void handle_remove_package(char command[], int *current_length, 
    int *max_size, bpkg_obj **list);

void handle_view_package(char command[], int *current_length, 
    int *max_size, bpkg_obj **list);

bpkg_obj *check_ident(char ident[], int current_length, bpkg_obj **list);

Chunk *request_hash(char hash[], bpkg_obj *obj);
