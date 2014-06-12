#define _GNU_SOURCE
#include "limits.h"
#include "stdio.h"
#include "string.h"
#include "stddef.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "errno.h"

const int EINT_RETRY_CNT = 5;

const int EXIT_SUCCESS_STATUS = 0;
const int EXIT_OTHER_ERROR_STATUS = 3;
const int EXIT_INVALID_ARGS_STATUS = 2;
const int EXIT_OPEN_ERROR_STATUS = 1;

const int STDIN_BUFFER_SIZE = PATH_MAX * 2 + 5000;

struct file_task{
    char * filename;
    int flags;
};

int get_fcount(int argc){
    return argc/2 - 1;
}

const int READ_PARAMS_EOF = 1;
const int READ_PARAMS_INVALID_ARGS = 2;
const int READ_PARAMS_OTHER = 4;

int is_delim_char(char ch, int read){
    if(read < 2) return ch == ' ';
    return ch == '\0' || ch == '\n';
}

int read_params(int * uid, int * _flags, char * filename, char * buffer, char ** caret){
    const int parts_cnt = 3;
    char * part_starts [parts_cnt];
    char * start = buffer;
    int parts_read = 0;
    char * pos = buffer;
    for(; parts_read < parts_cnt && pos < *caret; ++pos){
        if(is_delim_char(*pos, parts_read)){
            part_starts[parts_read++] = start;
            start = pos + 1;
        }
    }
    while(parts_read < parts_cnt){
        int len = STDIN_BUFFER_SIZE - (*caret - buffer);
        if(len == 0) {
            fprintf(stderr, "Too long line on input\n");
            return READ_PARAMS_OTHER;
        }
        int ret = read(STDIN_FILENO, *caret, len);
        if(ret == -1) return READ_PARAMS_OTHER;
        if(ret == 0) {
            **caret = 0;
            (*caret)++;
        }
        *caret = (*caret) + ret;
        for(; parts_read < parts_cnt && pos < *caret; ++pos){
            if(is_delim_char(*pos, parts_read)){
                part_starts[parts_read++] = start;
                start = pos + 1;
            }
        }
        if(ret == 0) break;
    }
    if(parts_read < parts_cnt){
        //we failed to read 3 parts
        if(parts_read > 0){
            fprintf(stderr, "You must pass 3 arguments per 0-terminated line\n");
            return READ_PARAMS_INVALID_ARGS ^ READ_PARAMS_EOF;
        }
        return READ_PARAMS_EOF;
    }
    *(start - 1) = 0;
    *(part_starts[1] - 1) = 0;
    *(part_starts[2] - 1) = 0;
    if(sscanf(part_starts[0], "%d", uid) == EOF){
        perror("Invalid uid argument");
        return READ_PARAMS_INVALID_ARGS;
    }
    strcpy(filename, part_starts[2]);
    char * mode = part_starts[1];
    int flags = 0;
    if(strcmp(mode, "r") == 0)
        flags |= O_RDONLY;
    else if(strcmp(mode, "w") == 0)
        flags |= O_WRONLY;
    else if(strcmp(mode, "rw") == 0)
        flags |= O_RDWR;
    else{
        fprintf(stderr, "Invalid mode: %s", mode);
        return READ_PARAMS_INVALID_ARGS;
    }
    *_flags = flags;
    for(char * p = start; p != *caret; p++)
        *(buffer + ((unsigned)(p - start))) = *p;
    *caret = buffer + ((unsigned)(*caret - start));
    return 0;
}

void handle_open_error(const char * filename){
    char errMsg[255 + PATH_MAX];
    sprintf(errMsg,"Openning file %s", filename);
    perror(errMsg);
}

int try_open(const char * filename, int flags){
    int fd;
    int retryCount = 0;
    do{
        ++retryCount;
        fd = open(filename, flags);
        if(fd == -1){
            int errNo = errno;
            if(errNo != EINTR){
                handle_open_error(filename);
                return EXIT_OPEN_ERROR_STATUS;
            }
        }
    }while (fd == -1 && retryCount < EINT_RETRY_CNT);
    if(fd == -1){
        handle_open_error(filename);
        return EXIT_OPEN_ERROR_STATUS;
    }
    int ret = close(fd);
    //We can simply forget about closing the file if error occurs, unless some limits exceed
    //it'll be closed on exit
    return EXIT_SUCCESS_STATUS;
}

int change_uids(uid_t * ruid, uid_t * euid, uid_t * suid, uid_t new_euid){
    int ret = getresuid(ruid, euid, suid);
    if(ret == -1){
        perror(NULL);
        return EXIT_OTHER_ERROR_STATUS;
    }
    ret = setresuid(*ruid, new_euid, *euid);
    if(ret == -1){
        perror(NULL);
        return EXIT_OTHER_ERROR_STATUS;
    }
    return EXIT_SUCCESS_STATUS;
}

int restore_uids(uid_t ruid, uid_t euid, uid_t suid){
    int ret = setresuid(ruid, euid, suid);
    if(ret == -1){
        perror(NULL);
        return EXIT_OTHER_ERROR_STATUS;
    }
    return EXIT_SUCCESS_STATUS;
}



int main(int argc, char ** argv){
    int uid, flags;
    char filename[PATH_MAX + 5];
    int ret;
    uid_t ruid, euid, suid;
    char buffer[STDIN_BUFFER_SIZE];
    char * caret = buffer;
    int read_params_ret;
    while(!(read_params_ret = read_params(&uid, &flags, filename, buffer, &caret))){
        if((ret = change_uids(&ruid, &euid, &suid, uid)) != EXIT_SUCCESS_STATUS)
            return ret; 
        if((ret = try_open(filename, flags)) != EXIT_SUCCESS_STATUS)
            return ret;
        if((ret = restore_uids(ruid, euid, suid)) != EXIT_SUCCESS_STATUS)
            return ret;
    }
    if(read_params_ret & READ_PARAMS_INVALID_ARGS){
        return EXIT_INVALID_ARGS_STATUS;
    }
    if(read_params_ret & READ_PARAMS_OTHER){
        return EXIT_OTHER_ERROR_STATUS;
    }
    return EXIT_SUCCESS_STATUS;
}
