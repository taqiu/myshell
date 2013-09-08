/*
 * P536-myshell
 * File: jobctrl.c 
 * job control routines
 *
 *
 * For each command (e.g. ls | sort; [foreground]  sleep 20 & [backrgound]),
 *  a job is created to hold 
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0
#define DEBUG 1
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

/* use a linked list to store all jobs */
job *job_head = NULL;

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


process* add_process(process *proc_head, char *cmd) {
    process *new_proc, *p;
    char *file_in, *file_out;
    int append, argc;

    /* create new proc */
    new_proc = (process*) malloc(sizeof(process));
    new_proc->next = NULL;
    new_proc->done = FALSE;
    /* parse args */
    argc = parse_cmd(cmd, new_proc->argv, &file_in, &file_out, &append);
    if (argc == 0)
        return proc_head;  
#if DEBUG
    printf("[debug] add process: %s, file in: %s, file out: %s, append mode: %d\n", new_proc->argv[0], file_in, file_out, append);
#endif

    /* set std in */
    if (file_in != NULL) {
        new_proc->std_in = open(file_in, O_RDONLY);    
    } else {
        new_proc->std_in = STDIN_FILENO;
    }
    if (new_proc->std_in < 0) {
        perror("can't access");
    }

    /* set std out */
    if (file_out != NULL) {
        if (append)
            new_proc->std_out = open(file_out, O_RDWR | O_CREAT | O_APPEND, 0664);
        else
            new_proc->std_out = open(file_out, O_RDWR | O_CREAT, 0664);
    } else {
        new_proc->std_out = STDOUT_FILENO;
    }
    if (new_proc->std_out < 0) {
        perror("can't access");
    }


    if (proc_head == NULL) {
        proc_head = new_proc;
    } else {
        p = proc_head;
        while(p) {
            if (p->next == NULL) {
                p->next = new_proc;
                break;
            }
            p = p->next;
        }
    }

    return proc_head;
}


process* create_process_list(char *cmds) {
    process* proc_head = NULL;  
    char *cmd;
    cmd = strtok(cmds, "|");
    while (cmd != NULL) {
        proc_head = add_process(proc_head, cmd);
        cmd = strtok(NULL, "|");
    }

    return proc_head;
}


void add_new_job(char *command, int foreground) {
    job *new_job, *j;

#if DEBUG
    printf("[debug] add job: \" %s \" \n", command);
#endif

    /* create a new job */
    new_job = (job*) malloc(sizeof(job));
    new_job->next = NULL;
    new_job->started = FALSE;
    new_job->foreground = foreground;
    /* assign a new space for command */
    new_job->command = (char*) malloc((strlen(command) + 1)*sizeof(char));
    strcpy(new_job->command, command);
    new_job->cmds = (char*) malloc((strlen(command) + 1)*sizeof(char));
    strcpy(new_job->cmds, command);
    /* add processes */
    new_job->proc_head = create_process_list(new_job->cmds);

    /* add new job to job list */
    if (job_head == NULL) {
        job_head = new_job;
        job_head->id = 1;
    } else {
        j = job_head;
        while (j) {
            if (j->next == NULL) {
                new_job->id = j->id + 1;
                j->next = new_job;
                break;
            }
            j = j->next;
        }
    }
}



void free_job(job *j) {
    free(j->command);
    free(j->cmds);
    free(j);
}


int is_job_completed(job *j) {
    process *p;
    for (p = j->proc_head; p; p = p->next) {
        if (p->done != TRUE) {
            return FALSE;   
        }   
    }
    return TRUE; 
}


void remove_completed_jobs() {
    job *j, *next;
    
    /* handle the fisrt job node */ 
    while (job_head) {
        if (is_job_completed(job_head)) {
            next = job_head->next;
            free_job(job_head);
            job_head = next;
        } else 
            break;
    }

    if (job_head == NULL) 
        return;

    /* handle job nodes except the first node */
    j = job_head;
    while (j->next) {
        if (is_job_completed(j->next)) {
            next = j->next;
            j->next = next->next;
            free(next);
        } else 
            j = j->next;
    }
}


void free_job_list() {
    job *j, *next;
    j = job_head;
    while(j) {
        next = j->next;
        free_job(j);
        j = next;
    }
}


void set_proc_done(pid_t pid, int status) {

#if DEBUG
    printf("[debug] process[%d] done: %d\n", pid, status);
#endif

    job *j;
    process *p;

    j = job_head;
    while (j) {
        p = j->proc_head;
        while(p) {
            if (p->pid == pid) {
                p->done = TRUE;
                p->status = status;
            }
            p = p->next;
        }
        j = j->next;
    }
}

void update_job_status() {
    int status;
    pid_t pid;

    while(TRUE) {
        pid = waitpid(WAIT_ANY, &status, WNOHANG);
        if (pid <= 0)
            break;
        else 
            set_proc_done(pid, status);
    }
}

void wait_for_job(job *j) {
    do {
        update_job_status();
    } while(!is_job_completed(j));
}


void launch_process(process *p, int file_in, int file_out) {
    /* set std in */
    if (p->std_in != STDIN_FILENO) {
        dup2(p->std_in, STDIN_FILENO);
        close(p->std_in);
    } else if (file_in != STDIN_FILENO) {
        dup2(file_in, STDIN_FILENO);
        close(file_in);
    }

    /* set std out */
    if (p->std_out != STDOUT_FILENO) {
        dup2(p->std_out, STDOUT_FILENO);
        close(p->std_out);
    } else if (file_out != STDOUT_FILENO) {
        dup2(file_out, STDOUT_FILENO);
        close(file_out);
    }

    if (execvp(p->argv[0], p->argv) < 0) {
        perror("failed to exec");
        exit(1);
    }
}


void launch_job(job *j) {
    process *p;
    pid_t pid;
    int fd[2], file_in, file_out;

#if DEBUG
    printf("[debug] launch job[%d]: %s \n", j->id, j->command);
#endif

    p = j->proc_head;
    file_in = STDIN_FILENO;
    while(p) {
        /* set pipes */
        if (p->next) {
            if (pipe(fd) < 0) {
                perror("pipe");
                exit(1);
            }
            file_out = fd[1];
        } else
            file_out = STDOUT_FILENO;
    
        /* execute in child processes */
        pid = fork();
        if (pid < 0 ) {
            perror("can't create child proccess");
            exit(1);
        } else if (pid == 0) {
            /* launch a child process */
            launch_process(p, file_in, file_out);
        } else {
            /* set the pid of child process */ 
            p->pid = pid;
#if DEBUG
    printf("[debug] launch proess[%d]: %s \n", p->pid, p->argv[0]);
#endif
        }

        /* close opening files in main process */
        if (file_in != STDIN_FILENO) {
            close(file_in);
        }
        if (file_out != STDOUT_FILENO) {
            close(file_out);
        }
        file_in = fd[0];

        p = p->next;
    }

    if (j->foreground) {
        wait_for_job(j);
    }
}

    
void run_jobs() {
    job *j;
    j = job_head;
    while(j) {
        if (!j->started)
            launch_job(j);       
        j = j->next;
    }
}



int main() {

    add_new_job("ls | sort >> 1123", TRUE);
    add_new_job("ls | sleep 1", TRUE);

    printf("=========== testing ===================\n");
    job *j;
    process *p;
    for(j = job_head; j; j = j->next) {
        printf("job [%d], cmd: %s, start: %d \n", j->id, j->command, j->started); 
        for (p = j->proc_head; p ; p= p->next) {
            printf("proc %s, stdin: %d, stdout: %d, done: %d\n", p->argv[0], p->std_in, p->std_out, p->done);
        }
        printf("---\n"); 
    }

    run_jobs();
    free_job_list();
}


