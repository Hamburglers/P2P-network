#include "config/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// function to start the btide program, parsing the config file
int parse_config(char *path, Config *cfg)
{
    // try opening config file path
    FILE *file = fopen(path, "r");
    if (!file)
    {
        fprintf(stderr, "Error opening file\n");
        return 1;
    }

    char buf[MAXLINELENGTH];
    int parsed[3] = {0};

    // keep parsing
    while (fgets(buf, sizeof(buf), file) != NULL)
    {
        // remove the new line
        buf[strcspn(buf, "\n")] = 0;
        if (strncmp(buf, "directory:", 10) == 0)
        {
            // check duplicate entry
            if (parsed[0] == 0)
            {
                parsed[0] = 1;
            }
            else
            {
                fprintf(stderr, "Duplicate entry for directory\n");
                return 1;
            }
            strncpy(cfg->directory, buf + 10, MAXLINELENGTH - 1);
            struct stat st = {0};
            // condition true if directory doesnt exist
            if (stat(cfg->directory, &st) == -1)
            {
                // 755 for only read and execute for group and others
                if (mkdir(cfg->directory, 0755) == -1)
                {
                    fprintf(stderr, "Failed to create directory\n");
                    return 3;
                }
            }
            else
            {
                // check if its a file
                if (!S_ISDIR(st.st_mode))
                {
                    fprintf(stderr, "The specified path is not a directory\n");
                    return 3;
                }
                // else directory exists
            }
        }
        else if (strncmp(buf, "max_peers:", 10) == 0)
        {
            // check duplicate entry
            if (parsed[1] == 0)
            {
                parsed[1] = 1;
            }
            else
            {
                fprintf(stderr, "Duplicate entry for max_peers\n");
                return 1;
            }
            cfg->max_peers = atoi(buf + 10);
            // check within bounds [1, 2048]
            if (cfg->max_peers < 1 || cfg->max_peers > 2048)
            {
                fprintf(stderr, "Invalid number parsed for max_peers\n");
                fclose(file);
                return 4;
            }
        }
        else if (strncmp(buf, "port:", 5) == 0)
        {
            // check duplicate entry
            if (parsed[2] == 0)
            {
                parsed[2] = 1;
            }
            else
            {
                fprintf(stderr, "Duplicate entry for port\n");
                return 1;
            }
            cfg->port = atoi(buf + 5);
            // check within bound (1024, 65535]
            if (cfg->port <= 1024 || cfg->port > 65535)
            {
                fprintf(stderr, "Invalid number parsed for port\n");
                fclose(file);
                return 5;
            }
        }
        else
        {
            // unknown field
            fprintf(stderr, "Unknown field parsed\n");
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    // check if all fields have been parsed
    for (int i = 0; i < 3; i++)
    {
        if (parsed[i] != 1)
        {
            fprintf(stderr, "Missing field detected in config\n");
            return 1;
        }
    }
    return 0;
}