#include "epoller.h"

Epoller::Epoller(int max_events_num_) :ready_events(max_events_num_){
    epollfd = epoll_create(5);
    assert(epollfd >= 0 && max_events_num_ > 0);
}

Epoller::~Epoller() {
    close(epollfd);
}

bool Epoller::Addfd(int fd, uint32_t event, bool oneshot) {
    if (fd < 0) {
        return false;
    }

    struct epoll_event new_ev;
    new_ev.data.fd = fd;

    if (oneshot) {
        new_ev.events |= EPOLLONESHOT;
    }

    new_ev.events = event | EPOLLET | EPOLLRDHUP;

    int ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &new_ev);

    return ret == 0;
}

bool Epoller::Delfd(int fd) {
    if (fd < 0) {
        return false;
    }

    return 0 == epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
}

bool Epoller::Modfd(int fd, uint32_t event) {
    if (fd < 0) {
        return false;
    }

    struct epoll_event new_event;
    new_event.data.fd = fd;
    new_event.events = event | EPOLLRDHUP | EPOLLONESHOT | EPOLLET;

    return 0 == epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &new_event);
}

int Epoller::Wait(int timeout) {
    return epoll_wait(epollfd, &ready_events[0], static_cast<int>(ready_events.size()), timeout);
}

int Epoller::Getfd(size_t i) {
    assert(i >= 0 && i < ready_events.size());

    return ready_events[i].data.fd;
}
uint32_t Epoller::Getevent(size_t i) {
    assert(i >= 0 && i < ready_events.size());
    
    return ready_events[i].events;
}