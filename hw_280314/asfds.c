#include "sys/types.h"
#include "sys/stat.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"
#include "wait.h"
#include "fcntl.h"

int main(int argc, char **argv) {
    char *prog_name = argv[1];
    int fd[argc - 2];
    for (int i = 0; i < argc - 2; ++i) {
        char *filepath = argv[i + 2];
        fd[i] = open(filepath, O_RDONLY);
        if (fd[i] < 0) {
            char err[255];
            sprintf(err, "Openning file %s", filepath);
            perror(err);
            return EXIT_FAILURE;
        }
    }
    char buffer[argc - 2][30];
    char *new_argv[argc];
    new_argv[0] = prog_name;
    for (int i = 0; i < argc - 2; ++i) {
        new_argv[i + 1] = buffer[i];
        sprintf(new_argv[i + 1], "%d", fd[i]);
    }
    new_argv[argc - 1] = 0;

    char *envp[1];
    envp[0] = 0;

    pid_t pid = fork();
    if (pid == 0) {
        int ret = execve(prog_name, new_argv, envp);
        if (ret == -1) {
            char err[255];
            sprintf(err, "Executing %s", prog_name);
            perror(err);
            return EXIT_FAILURE;
        }
    } else if (pid == -1) {
        perror(0);
        return EXIT_FAILURE;
    } else {
        int status = 0;
        pid_t ret = waitpid(pid, &status, 0);
        if (ret == -1) {
            char err[255];
            sprintf(err, "Waiting for pid %d", pid);
            perror(err);
            return EXIT_FAILURE;
        }
        for (int i = 0; i < argc - 2; ++i) {
            int ret2 = close(fd[i]);
            if (ret2 == -1) {
                char err[255];
                sprintf(err, "Closing fd #%d", i+1);
                perror(err);
                return EXIT_FAILURE;
            }
        }
    }
    return EXIT_SUCCESS;
}
