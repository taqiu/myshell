#ifndef PARSER_H
#define PARSER_H

#include "myshell.h"

char* strip(char* str);
char*  parse_input(char* input, int *foreground);
int parse_cmd(char *cmd, char **argv, char **file_in, char **file_out, int *append);

#endif
