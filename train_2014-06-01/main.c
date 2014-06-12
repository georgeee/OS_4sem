#include "io.h"
#include "str.h"
#include "complex_pipe.h"

#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "fcntl.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/wait.h"

struct line_read_state{
    struct complex_pipe_t pipe;
    int line_counter;
};

struct line_read_state init_line_read_state(){
    struct line_read_state res;
    res.line_counter = 0;
    return res;
};

int line_processor (struct line_read_state * state, char * line, int len, int counter, int err_code){
    if(err_code != _GETLINE_NONE && err_code != _GETLINE_EOF) return -1;
    if(len > 0){
        state->line_counter++;
        if(state->line_counter == 1){
            return init_complex_pipe(&state->pipe, create_exec_args(line));
        }else{
            struct str_splited_pair pointers [3];
            int cnt = _split(line, " ", pointers, 2);
            if(cnt < 2){
                _print(STDERR_FILENO, "Wrong format!\n");
                return 0;
            }
            *pointers[0].end = '\0';
            *pointers[1].end = '\0';
            if(strcmp(pointers[0].start, "->") == 0){
                return bind_left(&state->pipe, create_exec_args(pointers[1].start));
            }else if(strcmp(pointers[0].start, "<-") == 0){
                return bind_right(&state->pipe, create_exec_args(pointers[1].start));
            }else{
                _print(STDERR_FILENO, "Wrong format!\n");
                return 0;
            }
        }
    }else{
        return 1;
    }
    return 0;
}

int main(){
    struct line_read_state state = init_line_read_state();
    int stdin_backup = dup(STDIN_FILENO);
    int stdout_backup = dup(STDOUT_FILENO);
    int ret = fcntl(stdin_backup, F_SETFD, FD_CLOEXEC);
    if(ret < 0) return EXIT_FAILURE;
    ret = fcntl(stdout_backup, F_SETFD, FD_CLOEXEC);
    if(ret < 0) return EXIT_FAILURE;
    ret = _read_lines(stdin_backup, &state, (int (*)(void *, char *, int, int, int))&line_processor);
    if(ret < 0) return EXIT_FAILURE;
    ret = open("/dev/null", O_RDWR);
    if(ret < 0) return EXIT_FAILURE;
    int dev_null_fd = ret;
    if(state.line_counter){
        ret = finalize_input(&state.pipe, dev_null_fd);
        if(ret < 0) return EXIT_FAILURE;
        ret = finalize_output(&state.pipe, stdout_backup);
        if(ret < 0) return EXIT_FAILURE;
    }
    //fprintf(stderr, "Everything is set\n");
    wait(NULL);
    return EXIT_SUCCESS;
}
