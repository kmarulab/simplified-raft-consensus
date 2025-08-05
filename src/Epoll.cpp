#include "Epoll.h"
#include <unistd.h>
#include <stdexcept>

Epoll::Epoll() {
    m_epollFd = epoll_create1(0);
    if (m_epollFd == -1) {
        throw std::runtime_error("Failed to create epoll instance");
    }
}

Epoll::~Epoll() {
    if (m_epollFd != -1) {
        ::close(m_epollFd);
    }
}

void Epoll::add(int fd, uint32_t events) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &event) == -1) {
        throw std::runtime_error("Failed to add fd to epoll");
    }
}

void Epoll::modify(int fd, uint32_t events) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
    if (epoll_ctl(m_epollFd, EPOLL_CTL_MOD, fd, &event) == -1) {
        throw std::runtime_error("Failed to modify fd in epoll");
    }
}

void Epoll::remove(int fd) {
    if (epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        // Can fail if fd is already closed, not a fatal error
    }
}

int Epoll::wait(struct epoll_event* events, int max_events, int timeout) {
    return epoll_wait(m_epollFd, events, max_events, timeout);
}