#ifndef ARG_TYPES_H
#define ARG_TYPES_H
#include "threadPool.h"

// This file is used to define the types of params of functions to
//  unify the interface of functions to be put in the thread pool.

// example:
// void func(void* arg) {
//     struct save_file_arg* savefile_arg = (struct save_file_arg*) arg;

typedef struct ftp_session_arg
{
    int sock_cmd;
    char rootWorkDir[MAXSIZE];
    ThreadPool *pool;
} ftp_session_arg;

typedef struct process_command_arg
{
    int sock_cmd;
    char cmd[MAXSIZE];
    char arg[MAXSIZE];
    char cwd[MAXSIZE];
    char rootWorkDir[MAXSIZE];
    int *dataLinkEstablished;
    int *sock_data;
    pthread_mutex_t *mutex_data;
    int *passive_mode;
    int *transfer_type;
} process_command_arg;

typedef struct listen_pasv_arg
{
    int *sock_data;
    int *dataLinkEstablished; 
    int *passive_mode;
    ThreadPool *pool;
    pthread_mutex_t *mutex
} listen_pasv_arg;

#endif