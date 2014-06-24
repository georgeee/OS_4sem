#ifndef IO_H
#define IO_H

#include "string.h"
#include "unistd.h"

#define BUF_SIZE (1024*1024)
struct buf_t {
    char data[BUF_SIZE];
    int cursor;
    int length;
};

struct buf_t init_buf(){
    struct buf_t buf;
    buf.cursor = buf.length = 0;
    return buf;
}

int _getchar(int fd, struct buf_t * buf){
    if(buf->cursor >= buf->length){
        int ret = read(fd, buf->data, BUF_SIZE);
        if(ret < 0) return -2;
        if(ret == 0) return -1;
        buf->length = ret;
        buf->cursor = 0;
    }
    return buf->data[buf->cursor++];
}

const int _GETLINE_EOF = 1;
const int _GETLINE_ERR = 2;
const int _GETLINE_OVERFLOW = 3;
const int _GETLINE_NONE = 0;
int _getline(int fd, struct buf_t * buf, char * line, int max_size, int * err_code){
    int i = 0;
    if(err_code)
        *err_code = _GETLINE_NONE;
    while(i < max_size - 1){
        int chr = _getchar(fd, buf);
        if(chr == '\n' || chr == -1){
            if(chr == -1)
                if(err_code)
                    *err_code = _GETLINE_EOF;
            line[i] = '\0';
            return i;
        }
        if(chr == -2){
            if(err_code)
                *err_code = _GETLINE_ERR;
            return -i - 1;
        }
        line[i++] = chr;
    }
    if(err_code)
        *err_code = _GETLINE_OVERFLOW;
    return -i - 1;
}

#define _READ_LINES_LINE_SIZE (BUF_SIZE)
int _read_lines(int fd, void * data, int (*line_processor) (void * /*data*/, char * /*line*/, int /*len*/, int /*counter*/, int /*err_code*/)){
    struct buf_t buf = init_buf();
    int counter = 0;
    char line[_READ_LINES_LINE_SIZE];
    int err_code = _GETLINE_NONE;
    while(err_code == _GETLINE_NONE){
        int len = _getline(fd, &buf, line, _READ_LINES_LINE_SIZE, &err_code);
        int status = line_processor(data, line, len, counter++, err_code);
        if(status) return status;
    }
    return 0;
}


int _write(int fd, const void *data, int size){
    int i = 0;
    while(i < size){
        int ret = write(fd, data + i, size - i);
        if(ret <= 0) return -i - 1;
        i += ret;
    }
    return i;
}

int _print(int fd, const char * line){
    return _write(fd, line, strlen(line)*sizeof(const char));
}

#endif
