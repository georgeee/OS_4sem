#include <cstdio>
#include "rw_io_service.hpp"
#include <condition_variable>
#include <chrono>
#include <thread>
#include <cassert>
#include <iostream>
#include <cstdio>
#include <cstdlib>

int main(){
    std::mutex mtx;
    std::condition_variable c_var;
    volatile bool init_finished = false;
    auto func = [&mtx, &c_var, &init_finished] (int msec, int out_fd){
        rw_io_service _service;
        string_async_writer writer(out_fd, _service);
        char buffer [1024];
        sprintf(buffer, "Awake after %d msec\n", msec);
        writer.write_all_async(buffer, [msec](ssize_t wrote){
            fprintf(stderr, "%d: wrote %zd chars\n", msec, wrote);
            return 1 + msec;
        });
        {
            std::unique_lock<std::mutex> lk(mtx);
            c_var.wait(lk, [&init_finished] {return init_finished; });
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(msec));
        fprintf(stderr, "%d: service.run() will be called now\n", msec);
        int ret = _service.run();
        assert(ret == 1 + msec);
        close(out_fd);
    };
    rw_io_service _service;
    volatile int counter = 0;
    const int thread_count = 3;
    char result[thread_count][2048];
    std::vector<std::thread> threads;
    for(int i = 0; i < thread_count; ++i){
        int pipefds[2];
        int ret = pipe(pipefds);
        if(ret < 0) {
            perror(NULL);
            exit(EXIT_FAILURE);
        }
        threads.push_back(std::thread(func, i * 50, pipefds[1]));
        string_async_reader reader(pipefds[0], _service);
        char * out_buffer = result[i];
        reader.read_cstring_async([i, &counter, out_buffer](char * buffer, bool is_eof, ssize_t count) {
                sprintf(out_buffer, "%d : read '%s' is_eof=%d strlen=%zu count=%zd\n", i, buffer, is_eof, strlen(buffer), count);
                if((++counter) == thread_count) return 1234;
                return 0;
        });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    {
        std::lock_guard<std::mutex> lk(mtx);
        init_finished = true;
        c_var.notify_all();
    }
    int ret = _service.run();
    assert(ret == 1234);
    for(std::thread & _thread : threads) _thread.join();
    for(int i = 0; i < thread_count; ++i){
        std::cout << result[i];
    }
    return EXIT_SUCCESS;
}

