#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stdint.h>

#define MAXLINELENGTH 128

typedef struct
{
    char directory[MAXLINELENGTH];
    int max_peers;
    uint16_t port;
} Config;

int parse_config(char *filename, Config *cfg);

#endif