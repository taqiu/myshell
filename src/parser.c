#include "parser.h"

char* strip(char* str) {
    char *end;

    while (*str == ' ') str++;
    if (*str == '\n') {
        *str = '\0';     
        return str;
    }
  
    end = str + strlen(str) - 1;
    while(end > str && (*end == ' ' || *end == '\n')) end--;
    *(end+1) = '\0';
    return str;
}

char*  parse_input(char* input, int *foreground) {
    static char *last;
    char *tok;
    *foreground = TRUE;

    if (input == NULL && (input = last) == NULL) 
        return NULL;

    /* skip leading '&' and ';' */
    while (*input == '&' || *input == ';') 
        *input++ = '\0'; 

    /* return at the end of string */
    if (*input == '\0')
        return NULL; 

    tok = input;
    while (*input != '&' && *input != ';' && *input != '\0') 
        input++;

    if (*input == '&')
        *foreground = FALSE;

    if (*input == '\0')
        last = NULL;
    else  {
        *input = '\0';
        last = ++input; 
    }

#if DEBUG    
    printf("[debug] token: %s, foreground: %d\n", tok, *foreground);
#endif

    return tok;
}

int parse_cmd(char *cmd, char **argv, char **file_in, char **file_out, int *append) {
    /* parse argv */
    int argc = 0;
    int mode = 0; /* 0:argv  1: file in mode  2:file out mode */
     
    *file_in = NULL;
    *file_out = NULL;
    *append   = FALSE;

    while (TRUE) {
        /* fill all space and tab with '\0' */
        while (*cmd == ' ' || *cmd == '\t')
            *cmd++ = '\0';
        /* break if reach the end of string */
        if (*cmd == '\0')
            break;

        /* set parse mode */
        if (*cmd == '<') {
            mode = 1; /* file in mode */
            *cmd++ = '\0';
            continue;
        }
        if (*cmd == '>') {
            mode = 2; /* file out mode */
            *cmd++ = '\0';
            if (*cmd++ == '>') {
                *append = TRUE;
                *cmd++ = '\0';
            } else {
                *append = FALSE;
            }
            continue;
        }

        if (mode == 1)
            *file_in = cmd;
        else if (mode == 2) {
            *file_out = cmd;
        } else {
            *argv++ = cmd;
            argc++;
        }

        /* move to next argument */
        while (*cmd != ' ' && *cmd != '\t' && *cmd != '\0'
                && *cmd != '<' && *cmd != '>')
            cmd++;
    }
    *argv = '\0';

    return argc;
}
