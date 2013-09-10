/*
 * P536 simple shell
 * Author: Tanghong Qiu
 * E-mail: taqiu@indiana.edu
 * File: myshell.c
 *
 */

#include "myshell.h"
#include "jobctrl.h"
#include "parser.h"

char prompt[20] = "myshell:";


void change_dir(char** argv) {
    if (argv[1] == NULL) {
        const char* home = getenv("HOME");
        /* stay in current dir if home is not defined */
        chdir(home? home:".");
    } else {
        if (chdir(argv[1]) < 0) {
            fprintf(stderr, "cd \'%s\': %s\n", argv[1], strerror(errno));
        }
    }
}


void type_prompt() {
    printf("%s", prompt);
}


void sigchld_handler(int s) {
    /* update job status and clean zombie processes */
    update_job_status();
}


int main(int argc, char **argv) {
    char buf[MAX_BUF_SIZE];
    int foreground;
    char *token, *input;
    struct sigaction sa;

    /* use sigaction instead of signal (signal is out of date)*/
    sa.sa_handler = sigchld_handler; /* reap all dead processes */
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }


    /* set prompt if specified */
    if (argc > 1) {
        strcpy(prompt, argv[1]);
    }

    while(TRUE) {
        type_prompt();

        /* get user input */
        if (fgets(buf, MAX_BUF_SIZE, stdin) != NULL) {
            /*strip whitespace and newline */
            input = strip(buf);
        } else {
            /* exit if ctrl + d */
            printf("\n");
            free_job_list();
            break;
        }

#if DEBUG
        printf("[debug] input: %s\n", input);
#endif

        remove_completed_jobs();
    

        if (strcmp(input, "exit") == 0) {
            /* we don't have to wait child processes, and 
             * those processes will be orphan processes. 
             * Just a friendly remind
             */
            if (are_all_jobs_done())
                break;
            else {
                printf("There are running jobs.\n");
                printf("Ctrl + d to force exit\n");
                continue;
            }
        } else if (strcmp(input, "jobs") == 0) {
            print_jobs();
            continue;
        }

        /* parse user input */
        token = parse_input(input, &foreground);
        while (token) {
            add_new_job(token, foreground);
            token = parse_input(NULL, &foreground);
        }    

        /* run jobs */
        run_jobs();
    }

    return 0;
}

