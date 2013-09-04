/*
 * P536 simple shell
 * Author: Tanghong Qiu
 * E-mail: taqiu@indiana.edu
 * File: myshell.c
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0
#define DEBUG 1 
#define MAX_BUF_SIZE 256

char prompt[20] = "myshell:";


int parse_input(char *input, char **argv) {
    int argc = 0;
    while (TRUE) {
        /* fill all space and tab with '\0' */
        while (*input == ' ' || *input == '\t') 
            *input++ = '\0';
        /* break if reach the end of string */
        if (*input == '\0')
            break;
        *argv++ = input;
        argc++;
        /* move to next argument */
        while (*input != ' ' && *input != '\t' && *input != '\0') 
            input++;
    }
    *argv = '\0';
    return argc;
}


void execute(char **argv) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid < 0 ) {
        perror("can't create child proccess");
        exit(1);
    } else if (pid == 0) {
        if (execvp(*argv, argv) < 0) {
            perror("failed to exec");
            exit(1);
        }
    } else {
        while (wait(&status) != pid);
    }
}

void change_dir(char** argv) {
    if (argv[1] == NULL) {
        const char* home = getenv("HOME");
        /* stay in current dir if home is not defined */
        chdir(home? home:".");
    } else {
        if (chdir(argv[1]) < 0) {
            perror("cd failed");
        }
    }
}

void type_prompt() {
    printf("%s", prompt);
}

int main(int argc, char **argv) {
    char input[MAX_BUF_SIZE];
    char *cmd_argv[64];
    char cmd_argc = 0;

    /* set prompt if specified */
    if (argc > 1) {
        strcpy(prompt, argv[1]);
    }

    while(TRUE) {
        type_prompt();

        /* get user input */
        if (fgets(input, MAX_BUF_SIZE, stdin) != NULL) {
            /* remove newline */
            char *newline = strchr(input, '\n');
            if (newline != NULL) {
                *newline = '\0';
            }
        } else {
            /* continue if ctrl + d */
            printf("\n");
            continue;
        }

        /* parse user input */
        cmd_argc = parse_input(input, cmd_argv);
        /* continue if blank input */
        if (cmd_argc == 0) {
            continue;
        }

#if DEBUG
        printf("[debug] input: %s\n", input);
        int i;
        printf("[debug] argc: %d\n", cmd_argc);
        for (i = 0; i < cmd_argc; i++) {
            printf("[debug] argv[%d]  %s\n", i, *(cmd_argv+i));
        }
#endif /* DEBUG */

        if (strcmp(*cmd_argv, "exit") == 0) {
            /* exit */
            break;
        } else if (strcmp(*cmd_argv, "cd") == 0) {
            change_dir(cmd_argv);
        } else {
            /* execute command */
            execute(cmd_argv);
        }
    }

    return 0;
}
