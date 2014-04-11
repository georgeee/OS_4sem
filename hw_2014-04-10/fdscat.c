#include "unistd.h"
#include "limits.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "sys/mman.h"

const int BUFFER_SIZE = 1024;
const int WRITE_ZERO_TRY_COUNT = 10;
const int MMAP_BUFFERS_SIZE = 32;
const int MAX_BUFF_SIZE = INT_MAX/2;


void nop(void * data){}

void* raw_allocate(size_t size) {
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

int raw_deallocate(void * start_address, size_t total_size) {
    return munmap(start_address, total_size);
}

int write_impl(int fd, const char * buf, int len, int min_write_count, void (*err_handler) (void*), void * err_handler_data){
    int ret2;
    int remained = len;
    int zero_try_counter = 0;
    if(min_write_count <= 0) min_write_count = len;
    while(remained > 0){
        ret2 = write(fd, buf + len - remained, remained);
        if(ret2 < 0) {
            err_handler(err_handler_data);
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
        min_write_count -= ret2;
        if(min_write_count <= 0) break;
    }
    return len - remained;
}

int write_impl_no_err_hadler(int fd, const char * buf, int len, int min_write_count){
    return write_impl(fd, buf, len, min_write_count, nop, NULL);
}

int read_impl(int fd, char * buf, int len, void (*err_handler) (void*), void * err_handler_data){
    int ret = read(fd, buf, len);
    if(ret == -1) err_handler(err_handler_data);
    return ret;
}

int read_impl_no_err_hadler(int fd, char * buf, int len){
    return read_impl(fd, buf, len, nop, NULL);
}

void read_from_input_fd_err_handler(void * data){
    char err[255 + PATH_MAX];
    sprintf(err, "Reading fd #%d", *((int*)data) + 1);
    perror(err);
}

void write_to_stdout_err_handler(void * data){
    perror("Writing to STDOUT");
}

int bufcpy(char * buf, int len, char ** mmap_buffers, int * mmap_buffer_count, int * caret, int *buff_id, int PAGE_SIZE){
    int len1 = (PAGE_SIZE << *buff_id) - *caret;
    if (len1 > len) len1 = len;
    int i;
    for(i = 0; i < len1; ++i) mmap_buffers[*buff_id][i + *caret] = buf[i];
    *caret += len1;
    if(len > len1){
        (*buff_id)++;
        if(*buff_id == *mmap_buffer_count){
            int new_size = PAGE_SIZE << *buff_id;
            if(new_size >= MAX_BUFF_SIZE){
                const char * err_msg = "Max buffer size limit exceed: to long line";
                write_impl_no_err_hadler(STDERR_FILENO, err_msg, strlen(err_msg), 0);
                return EXIT_FAILURE;
            }
            void * allocated_memory = raw_allocate(new_size);
            if(allocated_memory == MAP_FAILED){
                perror("Can't allocate memory for line buffer");
                return EXIT_FAILURE;
            }
            mmap_buffers[*buff_id] = allocated_memory;
            (*mmap_buffer_count)++;
        }
        for(i = 0; i + len1 < len; ++i) mmap_buffers[*buff_id][i] = buf[i + len1];
        *caret = i;
    }
    return EXIT_SUCCESS;
}

void reverse_char_array(char * array, int len){
    int i;
    char c;
    for(i = 0; (i << 1) < len; ++i){
        c = array[i];
        array[i] = array[len - i - 1];
        array[len - i - 1] = c;
    }
}

int buf_print_reversed(char ** mmap_buffers, int * mmap_buffer_count, int *caret, int *buff_id, int PAGE_SIZE){
    int ret2;
    int _buff_id = *buff_id, _caret = *caret;
    do{
        reverse_char_array(mmap_buffers[_buff_id], _caret);
        _buff_id--;
        _caret = PAGE_SIZE << _buff_id;
    }while(_buff_id >= 0);
    do{
        ret2 = write_impl(STDOUT_FILENO, mmap_buffers[*buff_id], *caret, 0, write_to_stdout_err_handler, NULL);
        if(ret2 < 0)
            return EXIT_FAILURE;
        (*buff_id)--;
        *caret = PAGE_SIZE << (*buff_id);
    }while(*buff_id >= 0);
    (*buff_id)++;
    return EXIT_SUCCESS;
}

int process_fd_impl(int fd_id, int fd, char ** mmap_buffers, int * mmap_buffer_count, int PAGE_SIZE){
    char buf[BUFFER_SIZE];
    int caret = 0, buff_id = 0;
    int ret, ret2;
    int i, j;
    do{
        ret = read_impl(fd, buf, BUFFER_SIZE, read_from_input_fd_err_handler, &fd_id);
        if(ret < 0) return EXIT_FAILURE;
        for(i = 0; i < ret; ++i){
            if(buf[i] == '\n'){
                if(i > 0){
                    reverse_char_array(buf, i);
                    ret2 = write_impl(STDOUT_FILENO, buf, i, 0, write_to_stdout_err_handler, NULL);
                    if(ret2 < 0) return EXIT_FAILURE;
                    ret2 = buf_print_reversed(mmap_buffers, mmap_buffer_count, &caret, &buff_id, PAGE_SIZE);
                    if(ret2 != EXIT_SUCCESS) return ret2;
                }
                //Print \n
                ret2 = write_impl(STDOUT_FILENO, buf + i, 1, 0, write_to_stdout_err_handler, NULL);
                if(ret2 < 0) return EXIT_FAILURE;

                for(j = 0; i + j + 1 < ret; ++j) buf[j] = buf[i + j + 1];
                ret = j;
                i = 0;
            }
        }
        ret2 = bufcpy(buf, ret, mmap_buffers, mmap_buffer_count, &caret, &buff_id, PAGE_SIZE); 
        if(ret2 != EXIT_SUCCESS) return ret2;
    }while(ret != 0);
    ret2 = buf_print_reversed(mmap_buffers, mmap_buffer_count, &caret, &buff_id, PAGE_SIZE);
    if(ret2 != EXIT_SUCCESS) return ret2;
    return EXIT_SUCCESS;
}

int process_fd(int i, int fd, char * postfix, int postfix_len, int PAGE_SIZE){
    char * mmap_buffers[MMAP_BUFFERS_SIZE];
    int mmap_buffer_count = 1;
    int ret;
    char line_buffer[PAGE_SIZE];
    mmap_buffers[0] = &line_buffer[0];

    int return_value = process_fd_impl(i, fd, mmap_buffers, &mmap_buffer_count, PAGE_SIZE);

    if(return_value == EXIT_SUCCESS && postfix_len > 0){
        ret = write_impl(STDOUT_FILENO, postfix, postfix_len, 0, write_to_stdout_err_handler, 0);
        if(ret < 0) return_value = EXIT_FAILURE;
    }

    if(mmap_buffer_count > 1){
        int i;
        for(i = 1; i < mmap_buffer_count; ++i){
            ret = raw_deallocate(mmap_buffers[i], PAGE_SIZE << i);
            if(ret < 0){
                perror("Failed to deallocate line buffer memory");
                return_value = EXIT_FAILURE;
                break;
            }
        }
    }
    return return_value;
}

int main(int argc, char ** argv){
    int PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
    char * delimeter = argv[1];
    int delimeter_len = strlen(delimeter);
    int fd_count = argc - 2;
    char ** fd_strs = argv + argc - fd_count;
    for(int i=0; i<fd_count; ++i){
        int fd;
        sscanf(fd_strs[i], "%d", &fd);
        int fd_res = process_fd(i, fd,
                i == fd_count - 1 ? "" : delimeter,
                i == fd_count - 1 ? 0  : delimeter_len, PAGE_SIZE);
        if(fd_res != EXIT_SUCCESS) return fd_res;
    }
    return EXIT_SUCCESS;
}
