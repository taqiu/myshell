#ifndef JOBCTRL_H
#define JOBCTRL_H

#include "myshell.h"
#include "parser.h"

#define MAX_ARGC 32

/* a running process for a command */
typedef struct process {
    struct process *next;
    pid_t pid;             /* process id */
    char *argv[MAX_ARGC];  /* command argv */
    int std_in;            /* standard in */
    int std_out;           /* standard out */
    int done;              /* TRUE if the execution is done */
    int status;            /* status of waitpid() */
} process;

/* a shell job which contains a set of process */
typedef struct job {
    struct job *next;
    int id;                /* job id */
    char* command;         /* job command */
    char* cmds;            /* store cmds for process */
    int started;           /* TRUE if the job is started */
    int foreground;        /* TRUE if the job is foregournd */
    process *proc_head;    /* process list */
} job;

void update_job_status();
void remove_completed_jobs();
void add_new_job(char* cmd, int foreground);
void run_jobs();
int are_all_jobs_done();
void print_jobs();

#endif
