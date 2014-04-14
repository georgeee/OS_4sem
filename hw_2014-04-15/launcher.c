#include "stdio.h"
#include "stddef.h"
#include "unistd.h"
#include "stdlib.h"

#define EXPLICIT_EXIT_SUCCESS (0)
#define EXPLICIT_EXIT_FAILURE (1)

int main(int argc, char ** argv){
    char *envp[1];
    envp[0] = 0;
    char * new_argv[argc - 1];
    for(int i = 2; i < argc; ++i){
        new_argv[i - 2] = argv[i];
    }
    new_argv[argc - 2] = 0;
    int ret = execve(argv[1], new_argv, envp);
    return EXPLICIT_EXIT_SUCCESS;
}
