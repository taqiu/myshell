#ifndef MYSHELL_H
#define MYSHELL_H

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define TRUE 1
#define FALSE 0
#define DEBUG 1 
#define MAX_BUF_SIZE 256

void change_dir(char** argv);

#endif
