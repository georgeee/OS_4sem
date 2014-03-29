#include "unistd.h"
#include "limits.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"

const int BUFFER_SIZE = 1024;
const int WRITE_ZERO_TRY_COUNT = 10;

int main(int argc, char ** argv){
    char * delimeter = argv[1];
    int delimeter_len = strlen(delimeter);
    int fd_count = argc - 2;
    char ** fd_strs = argv + argc - fd_count;
    char buf [BUFFER_SIZE];
    for(int i=0; i<fd_count; ++i){
        int fd;
        sscanf(fd_strs[i], "%d", &fd);
        char * wbuf = (i > 0) ? delimeter : "";
        int ret = (i > 0) ? delimeter_len : 0;
        do{
            if(ret == -1){
                char err[255 + PATH_MAX];
                sprintf(err, "Reading fd #%d", i+1);
                perror(err);
                return EXIT_FAILURE;
            }else{
                int ret2;
                int remained = ret;
                int zero_try_counter = 0;
                while(remained > 0){
                    ret2 = write(STDOUT_FILENO, wbuf + ret - remained, remained);
                    if(ret2 < 0) {
                        perror("Writing to STDOUT");
                        return EXIT_FAILURE;
                    }else if(ret2 == 0){
                        ++zero_try_counter;
                        if(zero_try_counter == WRITE_ZERO_TRY_COUNT){
                            return EXIT_FAILURE;
                        }
                    }else{
                        zero_try_counter = 0;
                    }
                    remained -= ret2;
                }
            }
            wbuf = buf;
            ret = read(fd, wbuf, BUFFER_SIZE);
        }while(ret != 0);
    }
    return EXIT_SUCCESS;
}
