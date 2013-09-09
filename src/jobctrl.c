/*
 * P536-myshell
 * File: jobctrl.c 
 * job control routines
 *
 */

#include "jobctrl.h"

/* use a linked list to store all jobs */
job *job_head = NULL;

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
        fprintf(stderr, "can't access \'%s\': %s\n", file_in, strerror(errno));
    }

    /* set std out */
    if (file_out != NULL) {
        if (append)
            new_proc->std_out = open(file_out, O_WRONLY | O_CREAT | O_APPEND, S_IREAD|S_IWRITE);
        else
            new_proc->std_out = open(file_out, O_WRONLY | O_CREAT, S_IREAD|S_IWRITE);
    } else {
        new_proc->std_out = STDOUT_FILENO;
    }
    if (new_proc->std_out < 0) {
        fprintf(stderr, "can't access \'%s\': %s\n", file_out, strerror(errno));
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
    process *p, *next;
    p = j->proc_head;
    while(p) {
        next = p->next;
        free(p);
        p = next;
    }

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
            printf("[%d]  done   %s\n", job_head->id, job_head->command);
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
            printf("[%d]  done   %s\n", j->next->id, j->next->command);
            next = j->next;
            j->next = next->next;
            free_job(next);
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
    job *j;
    process *p;

#if DEBUG
    printf("[debug] process[%d] done: %d\n", pid, status);
#endif

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

void remove_job(job *j) {
    job *cur;
    
    /* handle the fisrt job node */ 
    if (job_head->id == j->id) {
        job_head = j->next;
        free_job(j);
        return;
    }

    /* handle job nodes except the first node */
    cur = job_head;
    while (cur->next) {
        if (cur->next->id == j->id) {
#if DEBUG            
            printf("[debug] remove job[%d] %s\n", j->id, j->command);
#endif
            cur->next = j->next;
            free_job(j);
            break;
        } else 
            cur = cur->next;
    }
}

void wait_for_job(job *j) {
    do {
        update_job_status();
    } while(!is_job_completed(j));
    remove_job(j);
}


void launch_process(process *p, int file_in, int file_out) {
    /* set std in */
    if (p->std_in == -1) {
        /* write stdin to avoid block if can't open file*/
        fprintf(STDIN_FILENO, "\n");  
    } else if (p->std_in != STDIN_FILENO) {
        dup2(p->std_in, STDIN_FILENO);
        close(p->std_in);
    } else if (file_in != STDIN_FILENO) {
        dup2(file_in, STDIN_FILENO);
        close(file_in);
    }

    /* set std out */
    if (p->std_out == -1) {
        /* redirect stdout to null */
        int dev_null = open("/dev/null", O_WRONLY);
        dup2(dev_null, STDOUT_FILENO);
        close(dev_null);
    } else if (p->std_out != STDOUT_FILENO) {
        dup2(p->std_out, STDOUT_FILENO);
        close(p->std_out);
    } else if (file_out != STDOUT_FILENO) {
        dup2(file_out, STDOUT_FILENO);
        close(file_out);
    }

    if (execvp(p->argv[0], p->argv) < 0) {
        fprintf(stderr, "exec \'%s\': %s\n", p->argv[0], strerror(errno));
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
        /* change directory */
        if (strcmp(p->argv[0], "cd") == 0) {
            change_dir(p->argv);
            p->done = TRUE;
            p = p->next;
            continue;
         }    

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
    job *j, *next;
    j = job_head;
    while(j) {
        /* keep the next job first, because j can be removed after launched */
        next = j->next;
        if (!j->started) {
            launch_job(j);
            j->started = TRUE;
        }       
        j = next;
    }
}

int are_all_jobs_done() {
    if (job_head == NULL)
        return TRUE;
    else
        return FALSE;
}

void print_jobs() {
    job *j;
    for(j = job_head; j; j = j->next) {
        printf("[%d]  Running     %s\n", j->id, j->command); 
    }
}

/*
int main() {

    add_new_job("ls | sort >> 1123", TRUE);
    add_new_job("ls | sleep 3", FALSE);
    add_new_job("cat < adsasdf | sort", TRUE);


    run_jobs();

    while(TRUE) {

        if (are_all_jobs_done()) {
            break;   
        }
        update_job_status(); 
        remove_completed_jobs();   
    }        
    free_job_list(); 
}
*/


