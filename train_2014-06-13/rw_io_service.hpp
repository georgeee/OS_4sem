#ifndef RW_IO_SERVICE_HPP
#define RW_IO_SERVICE_HPP

#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>

#include <stdexcept>
#include <exception>
#include <functional>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <cstring>

#include "io_service.hpp"

#define IO_SERVICE_ERROR (-2)

class rw_io_service : public io_service {
    public:
    int read_async(int fd, void * buf, size_t count, std::function<int (ssize_t)> continuation, std::function<int (int)> err_handler){
        return subscribe(fd, EPOLLIN, [=] (int event_mask) {
            if(unsubscribeMask(fd, EPOLLIN) < 0) return IO_SERVICE_ERROR;
            ssize_t ret = read(fd, buf, count);
            if(ret < 0) return err_handler(errno);
            return continuation(ret);
        });
    }
    int write_async(int fd, const void * buf, size_t count, std::function<int (ssize_t)> continuation, std::function<int (int)> err_handler){
        return subscribe(fd, EPOLLOUT, [=] (int event_mask) {
            if(unsubscribeMask(fd, EPOLLOUT) < 0) return IO_SERVICE_ERROR;
            ssize_t ret = write(fd, buf, count);
            if(ret < 0) return err_handler(errno);
            int cret = continuation(ret);
            fprintf(stderr, "%p continuation returned %d\n", this, cret);
            return cret;
        });
    }
};

class rw_io_service_fd_utilite {
    protected:
        rw_io_service & service;
        const int fd;
        std::function<int (ssize_t, int)> err_handler;
        static int default_err_handler(ssize_t ret, int _errno){
            throw runtime_error(std::string("rw_io_service_fd_utilite: ret=") + std::to_string(ret) + " _errno=" + std::to_string(_errno));
        }
    public:
        void set_err_handler(std::function<int (ssize_t, int)> err_handler){
            this->err_handler = err_handler;
        }
        rw_io_service_fd_utilite(int fd, rw_io_service & service, std::function<int(ssize_t, int)> err_handler = default_err_handler)
            : fd(fd), service(service), err_handler(err_handler) {}
        virtual ~rw_io_service_fd_utilite(){}
};

class string_async_reader : public rw_io_service_fd_utilite {
    constexpr static const int BUFFER_SIZE = 1024 * 1024;
    constexpr static const int ERR_OVERFLOW = 1;
    char buffer[BUFFER_SIZE + 5];
    ssize_t buf_size;
    public:
        using rw_io_service_fd_utilite::rw_io_service_fd_utilite;
        //predicate function should return 0 to continue reading, i+1 to return
        //string [0..i], -k to report error whith code k
        int read_async_with_predicate(std::function<int (char *, bool /*is_eof*/, ssize_t /*index of last char*/)> continuation,
                std::function<int (char *, ssize_t /*total_size*/, ssize_t /*step*/)> predicate,
                std::function<int (ssize_t, int)> _err_handler = nullptr) {
            if(!_err_handler) _err_handler = err_handler;
            auto read_async_err_handler = [_err_handler] (int _errno) { return _err_handler(-1, _errno); };
            std::function<int(ssize_t)> cont;
            cont = [read_async_err_handler, this, continuation, predicate, _err_handler, cont] (ssize_t read_cnt){
                if(read_cnt == 0){
                    ssize_t _buf_size = buf_size;
                    buf_size = 0;
                    return continuation(buffer, true, _buf_size);
                }else if(read_cnt < 0){
                    return _err_handler(read_cnt, errno);
                }else{
                    ssize_t _buf_size = buf_size;
                    buf_size += read_cnt;
                    int _ret = predicate(buffer, buf_size, read_cnt);
                    if(_ret < 0) return _err_handler(-3, -_ret);
                    else if(_ret == 0){
                        if(BUFFER_SIZE - buf_size - 1 == 0) return _err_handler(-2, ERR_OVERFLOW);
                        return service.read_async(fd, buffer, BUFFER_SIZE - buf_size - 1, cont, read_async_err_handler);
                    }else{
                        int _res = continuation(buffer, false, _ret);
                        memcpy(buffer, buffer + _ret, buf_size - _ret);
                        buf_size -= _ret;
                        return _res;
                    }
                }
            };
            if(BUFFER_SIZE - buf_size - 1 == 0) return _err_handler(-2, ERR_OVERFLOW);
            return service.read_async(fd, buffer, BUFFER_SIZE - buf_size - 1, cont, read_async_err_handler);
        }
    public:
        int read_till_delim_async(std::function<int (char *, bool /*is_eof*/, ssize_t)> continuation, char delim = '\n',
                std::function<int (ssize_t, int)> _err_handler = nullptr){
            auto _continuation = [continuation] (char * buffer, bool is_eof, ssize_t i) {
                buffer[i] = '\0';
                return continuation(buffer, is_eof, i - 1);
            };
            auto read_till_delim_async_predicate = [delim] (char * buffer, ssize_t buf_size, ssize_t read_cnt){
                ssize_t _buf_size = buf_size - read_cnt;
                for(int i = _buf_size; i < buf_size; ++i){
                    if(buffer[i] == delim) {
                        return i + 1;
                    }
                }
                return 0;
            };
            return read_async_with_predicate(_continuation, read_till_delim_async_predicate, _err_handler);
        }
        int read_cstring_async(std::function<int (char *, bool /*is_eof*/, ssize_t)> continuation,
                std::function<int (ssize_t, int)> _err_handler = nullptr){
            return read_till_delim_async(continuation, '\0', _err_handler);
        }
        int read_line_async(std::function<int (char *, bool /*is_eof*/, ssize_t)> continuation,
                std::function<int (ssize_t, int)> _err_handler = nullptr){
            return read_till_delim_async(continuation, '\n', _err_handler);
        }
        int read_some_async(std::function<int (char *, bool /*is_eof*/, ssize_t)> continuation, std::function<int (ssize_t, int)> _err_handler = nullptr){
            return read_async_with_predicate(continuation, [] (char * buffer, ssize_t buf_size, ssize_t read_cnt) { return buf_size; }, _err_handler);
        }
        int read_amount_async(std::function<int (char *, bool /*is_eof*/, ssize_t)> continuation, ssize_t limit, std::function<int (ssize_t, int)> _err_handler = nullptr){
            return read_async_with_predicate(continuation, [limit] (char * buffer, ssize_t buf_size, ssize_t read_cnt) { return buf_size >= limit ? limit : 0; }, _err_handler);
        }
};

class string_async_writer : public rw_io_service_fd_utilite {
    public:
        using rw_io_service_fd_utilite::rw_io_service_fd_utilite;
        int write_all_async(const char * buffer, std::function<int (ssize_t)> continuation, ssize_t count = -1,
                std::function<int (ssize_t, int)> _err_handler = nullptr) {
            if(count < 0) count = strlen(buffer) + 1;
            using namespace std::placeholders;
            if(!_err_handler) _err_handler = err_handler;
            auto write_async_err_handler = [_err_handler] (int _errno) { return _err_handler(-1, _errno); };
            std::function<int(ssize_t, ssize_t)> cont;
            cont = [this, buffer, count, cont, continuation, _err_handler, write_async_err_handler] (ssize_t wrote, ssize_t ret) {
                if(ret > 0){
                    wrote += ret;
                    if(count > wrote){
                        return service.write_async(fd, buffer + wrote, count - wrote, std::bind(cont, wrote, _1), write_async_err_handler);
                    }
                    return continuation(wrote);
                }else{
                    return _err_handler(ret, errno);
                }
            };
            return service.write_async(fd, buffer, count, std::bind(cont, 0, _1), write_async_err_handler);
        }
        int write_some_async(const char * buffer, std::function<int (ssize_t)> continuation, ssize_t count = -1,
                std::function<int (ssize_t, int)> _err_handler = nullptr) {
            if(count < 0) count = strlen(buffer) + 1;
            if(!_err_handler) _err_handler = err_handler;
            auto write_async_err_handler = [_err_handler] (int _errno) { return _err_handler(-1, _errno); };
            std::function<int(ssize_t)> cont = [continuation, _err_handler] (ssize_t ret) {
                if(ret > 0){
                    return continuation(ret);
                }else{
                    return _err_handler(ret, errno);
                }
            };
            return service.write_async(fd, buffer, count, cont, write_async_err_handler);
        }

};







#endif
