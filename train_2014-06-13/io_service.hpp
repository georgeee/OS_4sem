#ifndef IO_SERVICE_HPP
#define IO_SERVICE_HPP

#include <sys/epoll.h>
#include <unistd.h>

#include <stdexcept>
#include <exception>
#include <functional>
#include <vector>
#include <unordered_map>
#include <memory>

using namespace std;

class io_service{
    int epfd;
    public:
        struct subsciption_holder : public vector<pair<int, std::function<int (int)> > >{
            int fd;
            subsciption_holder (int fd) : fd(fd) {}
            ~subsciption_holder(){}
        };
    private:
        std::unordered_map<int, unique_ptr<subsciption_holder> > subscriptions;
    public:
        io_service(){
            epfd = epoll_create(1);
            if(epfd < 0) throw runtime_error("Failed to create epoll fd");
        }
        //int read(int fd, void * buf, int size, std::function<void(int)> callback, std::function<void(int)> err_handler);
        //int write(int fd, void * buf, int size, std::function<void(int)> callback, std::function<void(int)> err_handler);
        int unsubscribeAll(int fd, std::function<void (const subsciption_holder &) > unsubcriber = nullptr){
            auto iter = subscriptions.find(fd);
            if(iter == subscriptions.end()) return -2;
            int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            if(ret < 0){
                return -1;
            }
            unique_ptr<subsciption_holder> sub = std::move(iter->second);
            subscriptions.erase(iter);
            if(unsubcriber) unsubcriber(*sub);
            return 0;
        }
        int call_by_mask(int fd, int mask){
            auto iter = subscriptions.find(fd);
            if(iter == subscriptions.end()) return 0;
            int state = 0;
            for(int i = 0; i < iter->second->size(); ++i){
                auto pr = iter->second->at(i);
                if(pr.first | mask){
                    int ret = pr.second(mask);
                    if(ret) state = ret;
                }
            }
            return state;
        }
        int run(){
            fprintf(stderr, "%p : run() launched\n", this);
            epoll_event event;
            while(true){
                int ret = epoll_wait(epfd, &event, 1, -1);
                if(ret < 0) return -1;
                if(ret){
                    int ret2 = call_by_mask(event.data.fd, event.events);
                    if(ret2){
                        fprintf(stderr, "%p : run() finished with %d\n", this, ret2);
                        return ret2;
                    }
                    else{
                        fprintf(stderr, "%p : run() continuing loop with %d\n", this, ret2);
                    }
                }
            }
            return 0;
        }
        int unsubscribeMask(int fd, int mask){
            auto iter = subscriptions.find(fd);
            if(iter == subscriptions.end()) return 1;
            auto & prev_holder = iter->second;
            unique_ptr<subsciption_holder> new_holder(new subsciption_holder(fd));
            int sum_mask = 0;
            for(int i = 0; i < prev_holder->size(); ++i){
                int & prev_mask = prev_holder->at(i).first; 
                prev_mask ^= prev_mask & mask;
                sum_mask |= prev_mask;
                if(prev_mask){
                    new_holder->push_back(prev_holder->at(i));
                }
            }
            subscriptions.insert(make_pair(fd, std::move(new_holder)));
            epoll_event event;
            epoll_data ev_data;
            ev_data.fd = fd;
            event.data = ev_data;
            event.events = sum_mask;
            int ret = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
            if(ret < 0) return -1;
            return 0;
        }
        int subscribe(int fd, int event_mask, std::function<int (int)> callback){
            auto & prev_holder = subscriptions.emplace(fd, unique_ptr<subsciption_holder>(new subsciption_holder(fd))).first->second;
            int sum_mask = 0;
            for(int i = 0; i < prev_holder->size(); ++i){
                int mask = prev_holder->at(i).first;
                if(mask & event_mask) return -2;
                sum_mask |= mask;
            }
            prev_holder->emplace_back(event_mask, callback);
            sum_mask |= event_mask;
            epoll_event event;
            epoll_data ev_data;
            ev_data.fd = fd;
            event.data = ev_data;
            event.events = sum_mask;
            if(prev_holder->size() == 1){
                int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
                if(ret < 0) return -1;
            }else{
                int ret = epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
                if(ret < 0) return -1;
            }
            return 0;
        }
        ~io_service(){
            close(epfd);
        }
};

#endif
