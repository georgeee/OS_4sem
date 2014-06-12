#ifndef COMPLEX_PIPE_H
#define COMPLEX_PIPE_H

#include "str.h"

#include "unistd.h"
#include "fcntl.h"
#include "stdlib.h"

struct complex_pipe_t{
    int left_fd; //fd for writing
    int right_fd; //fd for reading
};

#define MAX_ARG_COUNT (1005)
struct exec_args_t{
    const char * file;
    char * argv[MAX_ARG_COUNT];
};

int _execute(struct exec_args_t exec_args){
    int ret = fork();
    if(ret < 0) return -1;
    if(ret) return 0;
    ret = execvp(exec_args.file, exec_args.argv);
    if(ret < 0) exit(EXIT_FAILURE);
    return 0;
}


int init_complex_pipe(struct complex_pipe_t * complex_pipe, struct exec_args_t exec_args){
    int pipefds_left[2];
    int pipefds_right[2];
    int ret = pipe(pipefds_left);
    if(ret < 0) return -1;
    ret = pipe(pipefds_right);
    if(ret < 0) return -1;
    ret = fcntl(pipefds_left[0], F_SETFD, FD_CLOEXEC);
    if(ret < 0) return -1;
    ret = fcntl(pipefds_left[1], F_SETFD, FD_CLOEXEC);
    if(ret < 0) return -1;
    ret = fcntl(pipefds_right[0], F_SETFD, FD_CLOEXEC);
    if(ret < 0) return -1;
    ret = fcntl(pipefds_right[1], F_SETFD, FD_CLOEXEC);
    if(ret < 0) return -1;
    ret = dup2(pipefds_left[0], STDIN_FILENO);
    if(ret < 0) return -1;
    ret = dup2(pipefds_right[1], STDOUT_FILENO);
    if(ret < 0) return -1;
    complex_pipe->left_fd = pipefds_left[1];
    complex_pipe->right_fd = pipefds_right[0];
    ret = _execute(exec_args);
    if(ret < 0) return -1;
    ret = close(pipefds_left[0]);
    if(ret < 0) return -1;
    ret = close(pipefds_right[1]);
    if(ret < 0) return -1;
    //fprintf(stderr, "init_complex_pipe(complex_pipe : {l: %d, r: %d}, exec_args.file = %s)\n", complex_pipe->left_fd, complex_pipe->right_fd, exec_args.file);
    return 0;
}

int bind_right(struct complex_pipe_t * complex_pipe, struct exec_args_t exec_args){
    int pipefds_right[2];
    int ret = pipe(pipefds_right);
    if(ret < 0) return -1;
    ret = fcntl(pipefds_right[0], F_SETFD, FD_CLOEXEC);
    if(ret < 0) return -1;
    ret = fcntl(pipefds_right[1], F_SETFD, FD_CLOEXEC);
    if(ret < 0) return -1;
    ret = dup2(complex_pipe->right_fd, STDIN_FILENO);
    if(ret < 0) return -1;
    ret = dup2(pipefds_right[1], STDOUT_FILENO);
    if(ret < 0) return -1;
    //fprintf(stderr, "bind_right(complex_pipe : {l: %d, r: %d} => {l: %d, r: %d}, exec_args.file = %s)\n", complex_pipe->left_fd, complex_pipe->right_fd, complex_pipe->left_fd, pipefds_right[0], exec_args.file);
    complex_pipe->right_fd = pipefds_right[0];
    ret = _execute(exec_args);
    if(ret < 0) return -1;
    ret = close(pipefds_right[1]);
    if(ret < 0) return -1;
    return 0;
}

int bind_left(struct complex_pipe_t * complex_pipe, struct exec_args_t exec_args){
    int pipefds_left[2];
    int ret = pipe(pipefds_left);
    if(ret < 0) return -1;
    ret = fcntl(pipefds_left[0], F_SETFD, FD_CLOEXEC);
    if(ret < 0) return -1;
    ret = fcntl(pipefds_left[1], F_SETFD, FD_CLOEXEC);
    if(ret < 0) return -1;
    ret = dup2(pipefds_left[0], STDIN_FILENO);
    if(ret < 0) return -1;
    ret = dup2(complex_pipe->left_fd, STDOUT_FILENO);
    if(ret < 0) return -1;
    //fprintf(stderr, "bind_left(complex_pipe : {l: %d, r: %d} => {l: %d, r: %d}, exec_args.file = %s)\n", complex_pipe->left_fd, complex_pipe->right_fd, pipefds_left[1], complex_pipe->right_fd, exec_args.file);
    complex_pipe->left_fd = pipefds_left[1];
    ret = _execute(exec_args);
    if(ret < 0) return -1;
    ret = close(pipefds_left[0]);
    if(ret < 0) return -1;
    return 0;
}

int launch_inout_dumper(){
    struct exec_args_t args;
    args.file = "cat";
    char tmp[2][10];
    strcpy(tmp[0], "cat");
    strcpy(tmp[1], "-");
    args.argv[0] = tmp[0];
    args.argv[1] = tmp[1];
    args.argv[2] = NULL;
    return _execute(args);
}

int finalize_input(struct complex_pipe_t * complex_pipe, int fd){
    /*
     * We need to 1) bind stdin to fd
     * 2) bind stdout to complex_pipe's write end
     * 3) launch 'cat -'
     */
    int ret = dup2(fd, STDIN_FILENO);
    if(ret < 0) return -1;
    ret = dup2(complex_pipe->left_fd, STDOUT_FILENO);
    if(ret < 0) return -1;
    return launch_inout_dumper();
}

int finalize_output(struct complex_pipe_t * complex_pipe, int fd){
    /* We need to
     * 1) bind stdin to complex_pipe's read end
     * 2) bind stdout to fd
     * 3) launch 'cat -'
     */
    int ret = dup2(complex_pipe->right_fd, STDIN_FILENO);
    if(ret < 0) return -1;
    ret = dup2(fd, STDOUT_FILENO);
    if(ret < 0) return -1;
    return launch_inout_dumper();
}

struct exec_args_t create_exec_args(const char * cmd){
    struct exec_args_t args;
    struct str_splited_pair pointers[MAX_ARG_COUNT];
    int ret = _split(cmd, " ", pointers, MAX_ARG_COUNT);
    if(ret < 0) exit(EXIT_FAILURE);
    args.file = pointers[0].start;
    for(int i = 0; i < ret; ++i){
        args.argv[i] = pointers[i].start;
        *pointers[i].end = '\0';
    }
    args.argv[ret] = NULL;
    return args;
}


#endif
