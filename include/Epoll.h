#pragma once

#include <sys/epoll.h>
#include <vector>

class Epoll {
public:
    Epoll();
    ~Epoll();

    // Disable copy semantics
    Epoll(const Epoll&) = delete;
    Epoll& operator=(const Epoll&) = delete;

    void add(int fd, uint32_t events);
    void modify(int fd, uint32_t events);
    void remove(int fd);

    int wait(struct epoll_event* events, int max_events, int timeout);

private:
    int m_epollFd;
};