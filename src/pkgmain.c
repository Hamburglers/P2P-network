#define _POSIX_C_SOURCE 200112L
#include <chk/pkgchk.h>
#include <crypt/sha256.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

#define SHA256_HEX_LEN (64)

int arg_select(int argc, char **argv, int *asel, char *harg)
{

	char *cursor = argv[2];
	*asel = 0;
	if (argc < 3)
	{
		puts("bpkg or flag not provided");
		exit(1);
	}

	if (strcmp(cursor, "-all_hashes") == 0)
	{
		*asel = 1;
	}
	if (strcmp(cursor, "-chunk_check") == 0)
	{
		*asel = 2;
	}
	if (strcmp(cursor, "-min_hashes") == 0)
	{
		*asel = 3;
	}
	if (strcmp(cursor, "-hashes_of") == 0)
	{
		if (argc < 4)
		{
			puts("filename not provided");
			exit(1);
		}
		*asel = 4;
		strncpy(harg, argv[3], SHA256_HEX_LEN);
	}
	if (strcmp(cursor, "-file_check") == 0)
	{
		*asel = 5;
	}
	return *asel;
}

void bpkg_print_hashes(bpkg_query *qry)
{
	for (int i = 0; i < qry->len; i++)
	{
		printf("%.64s\n", qry->hashes[i]);
	}
}

int main(int argc, char **argv)
{

    // run using valgrind
    // if (getenv("RUNNING_UNDER_VALGRIND") == NULL) {
    //     setenv("RUNNING_UNDER_VALGRIND", "1", 1);
    //     char **new_argv = malloc((argc + 3) * sizeof(char *));
    //     if (new_argv == NULL) {
    //         perror("malloc");
    //         exit(EXIT_FAILURE);
    //     }

    //     new_argv[0] = "valgrind";
    //     new_argv[1] = "--leak-check=full";
    //     for (int i = 0; i < argc; i++) {
    //         new_argv[i + 2] = argv[i];
    //     }
    //     new_argv[argc + 2] = NULL;

    //     execvp("valgrind", new_argv);
    // }

	int argselect = 0;
	char hash[SHA256_HEX_LEN];

	if (arg_select(argc, argv, &argselect, hash))
	{
		bpkg_query qry = {0};
		bpkg_obj *obj = bpkg_load(argv[1]);

		if (!obj)
		{
			puts("Error: Unable to parse the '.bpkg' file. Check file integrity" 
				" and completeness.");
			exit(1);
		}

		if (argselect == 1)
		{
			if (bpkg_intialise_merkle(obj))
			{
				puts("Error: Unable to parse the '.bpkg' file. Check file "
					"integrity and completeness.");
				exit(1);
			}
			qry = bpkg_get_all_hashes(obj);
			bpkg_print_hashes(&qry);
			bpkg_query_destroy(&qry);
		}
		else if (argselect == 2)
		{
			if (bpkg_intialise_merkle(obj))
			{
				puts("Error: Unable to parse the '.bpkg' file. Check file "
					"integrity and completeness.");
				exit(1);
			}
			qry = bpkg_get_completed_chunks(obj);
			bpkg_print_hashes(&qry);
			bpkg_query_destroy(&qry);
		}
		else if (argselect == 3)
		{
			if (bpkg_intialise_merkle(obj))
			{
				puts("Error: Unable to parse the '.bpkg' file. Check file "
					"integrity and completeness.");
				exit(1);
			}
			qry = bpkg_get_min_completed_hashes(obj);
			bpkg_print_hashes(&qry);
			bpkg_query_destroy(&qry);
		}
		else if (argselect == 4)
		{
			if (bpkg_intialise_merkle(obj))
			{
				puts("Error: Unable to parse the '.bpkg' file. Check file "
					"integrity and completeness.");
				exit(1);
			}
			qry = bpkg_get_all_chunk_hashes_from_hash(obj, hash);
			bpkg_print_hashes(&qry);
			bpkg_query_destroy(&qry);
		}
		else if (argselect == 5)
		{
			// print_bpkg_obj(obj, obj->filename);
			// debug(obj->merkle);
			// char** q = levelOrderTraversal(obj);
			qry = bpkg_file_check(obj);
			if (bpkg_intialise_merkle(obj))
			{
				puts("Error: Unable to parse the '.bpkg' file. Check file "
					"integrity and completeness.");
				exit(1);
			}
			bpkg_print_hashes(&qry);
			bpkg_query_destroy(&qry);
		}
		else
		{
			puts("Argument is invalid");
			return 1;
		}
		bpkg_obj_destroy(obj);
	}

	return 0;
}