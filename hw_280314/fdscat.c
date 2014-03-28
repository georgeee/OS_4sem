#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"

const int BUFFER_SIZE = 1024;
const int WRITE_ZERO_TRY_COUNT = 10;

int main(int argc, char ** argv){
    char buf [BUFFER_SIZE];
    for(int i=0; i<argc; ++i){
        int fd;
        sscanf(argv[i + 1], "%d", &fd);
        int ret;
        while((ret = read(fd, buf, BUFFER_SIZE)) != 0){
            if(ret == -1){
                char err[255];
                sprintf(err, "Reading fd #%d", i+1);
                perror(err);
                return EXIT_FAILURE;
            }else{
                int ret2;
                int remained = ret;
                int zero_try_counter = 0;
                while(remained > 0){
                    ret2 = write(STDOUT_FILENO, buf + ret - remained, remained);
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
        }
    }
    return -1;
}
