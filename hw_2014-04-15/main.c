#define _GNU_SOURCE
#include "stdio.h"
#include "string.h"
#include "stddef.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "errno.h"

const int eintrRetryCount = 5;

const int exitSuccessStatus = 0;
const int exitOtherErrorStatus = 3;
const int exitInvalidArgsStatus = 2;
const int exitOpenErrorStatus = 1;

struct file_task{
    char * filename;
    int flags;
};

int get_fcount(int argc){
    return argc/2 - 1;
}

int read_args(int argc, char ** argv, int * uid, struct file_task * tasks){
    if(argc % 2 != 0) {
        fprintf(stderr, "You must pass 2n+1 argument");
        return exitInvalidArgsStatus;
    }
    if(sscanf(argv[1], "%d", uid) == EOF){
        perror("Invalid uid argument");
        return exitInvalidArgsStatus;
    }
    int fcount = get_fcount(argc);
    for(int i = 0; i < fcount; ++i){
        tasks[i].filename = argv[i*2 + 2];
        char * mode = argv[i*2 + 3];
        int flags = 0;
        if(strcmp(mode, "r") == 0)
            flags |= O_RDONLY;
        else if(strcmp(mode, "w") == 0)
            flags |= O_WRONLY;
        else if(strcmp(mode, "rw") == 0)
            flags |= O_RDWR;
        else{
            fprintf(stderr, "Invalid mode: %s", mode);
            return exitInvalidArgsStatus;
        }
        tasks[i].flags = flags;
    }
    return exitSuccessStatus;
}

void handle_open_error(const char * filename){
    char errMsg[255 + strlen(filename)];
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
                return exitOpenErrorStatus;
            }
        }
    }while (fd == -1 && retryCount < eintrRetryCount);
    if(fd == -1){
        handle_open_error(filename);
        return exitOpenErrorStatus;
    }
    int ret = close(fd);
    //We can simply forget about closing the file if error occurs, unless some limits exceed
    //it'll be closed on exit
    return exitSuccessStatus;
}

int change_uids(uid_t * ruid, uid_t * euid, uid_t * suid, uid_t new_euid){
    int ret = getresuid(ruid, euid, suid);
    if(ret == -1){
        perror(NULL);
        return exitOtherErrorStatus;
    }
    ret = setresuid(*ruid, new_euid, *euid);
    if(ret == -1){
        perror(NULL);
        return exitOtherErrorStatus;
    }
    return exitSuccessStatus;
}

int restore_uids(uid_t ruid, uid_t euid, uid_t suid){
    int ret = setresuid(ruid, euid, suid);
    if(ret == -1){
        perror(NULL);
        return exitOtherErrorStatus;
    }
    return exitSuccessStatus;
}

int main(int argc, char ** argv){
    int uid;
    int ret;
    int file_cnt = get_fcount(argc);
    struct file_task tasks[file_cnt];
    if((ret = read_args(argc, argv, &uid, tasks)) != exitSuccessStatus)
        return ret;
    uid_t ruid, euid, suid;
    if((ret = change_uids(&ruid, &euid, &suid, uid)) != exitSuccessStatus)
       return ret; 
    for(int i = 0; i < file_cnt; ++i){
        if((ret = try_open(tasks[i].filename, tasks[i].flags)) != exitSuccessStatus)
            return ret;
    }
    //In fact we don't have to restore permissions, but if we once need to, we
    //can do it by just uncommenting lines bellow
    /*if((ret = restore_uids(ruid, euid, suid)) != exitSuccessStatus)*/
        /*return ret;*/
    return exitSuccessStatus;
}
