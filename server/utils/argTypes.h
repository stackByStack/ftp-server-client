#ifndef ARG_TYPES_H
#define ARG_TYPES_H

//This file is used to define the types of params of functions to 
// unify the interface of functions to be put in the thread pool.

// example: 
// void func(void* arg) {
//     struct save_file_arg* savefile_arg = (struct save_file_arg*) arg;

typedef struct ftp_session_arg {
    int sock_cmd;
} ftp_session_arg;

#endif