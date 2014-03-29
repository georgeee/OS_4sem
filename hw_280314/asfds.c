#include "sys/types.h"
#include "limits.h"
#include "sys/stat.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"
#include "wait.h"
#include "fcntl.h"

int main(int argc, char **argv) {
    char *delimeter = argv[1];
    char *prog_name = argv[2];
    int fd_count = argc - 3;
    int fd[fd_count];
    char ** files = argv + argc - fd_count;
    for (int i = 0; i < fd_count; ++i) {
        char *filepath = files[i];
        fd[i] = open(filepath, O_RDONLY);
        if (fd[i] < 0) {
            char err[255 + PATH_MAX];
            sprintf(err, "Openning file %s", filepath);
            perror(err);
            return EXIT_FAILURE;
        }
    }
    char buffer[fd_count][30];
    char *new_argv[fd_count + 3];
    new_argv[0] = prog_name;
    new_argv[1] = delimeter;
    for (int i = 0; i < fd_count; ++i) {
        char * buf = new_argv[i + 2] = buffer[i];
        sprintf(buf, "%d", fd[i]);
    }
    new_argv[fd_count + 2] = 0;

    char *envp[1];
    envp[0] = 0;

    pid_t pid = fork();
    if (pid == 0) {
        int ret = execve(prog_name, new_argv, envp);
        if (ret == -1) {
            char err[255 + PATH_MAX];
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
            char err[255 + PATH_MAX];
            sprintf(err, "Waiting for pid %d", pid);
            perror(err);
            return EXIT_FAILURE;
        }
        for (int i = 0; i < fd_count; ++i) {
            int ret2 = close(fd[i]);
            if (ret2 == -1) {
                char err[255 + PATH_MAX];
                sprintf(err, "Closing fd #%d", i+1);
                perror(err);
                return EXIT_FAILURE;
            }
        }
    }
    return EXIT_SUCCESS;
}
