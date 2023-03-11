/*
 * @Author       : WKq
 * @Date         : 2022/09/07
 * @Desc         : epoll类
 */ 

#ifndef _EPOLLER_H
#define _EPOLLER_H

#include <sys/epoll.h>
#include <assert.h>
#include <unistd.h>
#include <vector>

class Epoller {
public:
    explicit Epoller(int max_events_num_ = 5000);
    ~Epoller();

    bool Addfd(int fd, uint32_t event, bool oneshot);   //EPOLLET、EPOLLRDHUP
    bool Delfd(int fd);
    bool Modfd(int fd, uint32_t event); //EPOLLET、EPOLLRDHUP、EPOLLONESHOT

    int Wait(int timeout);
    int Getfd(size_t i);
    uint32_t Getevent(size_t i);

private:
    int epollfd;       
    std::vector<struct epoll_event> ready_events;
};

#endif