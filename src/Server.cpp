// src/Server.cpp

#include "Server.h"
#include <iostream>
#include <unistd.h>
#include <vector>
#include <cerrno>
#include <sys/socket.h>

constexpr int MAX_EVENTS = 1024;
constexpr int BUFFER_SIZE = 4096;

Server::Server(uint16_t port, std::function<void(int, const std::string&)> handler)
    : m_port(port), 
      m_requestHandler(handler), 
      m_threadPool(std::thread::hardware_concurrency()) {}

void Server::run() {
    m_listenerSocket.create();
    m_listenerSocket.setNonBlocking();
    int opt = 1;
    setsockopt(m_listenerSocket.fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    m_listenerSocket.bind(m_port);
    m_listenerSocket.listen();

    m_epoll.add(m_listenerSocket.fd(), EPOLLIN | EPOLLET);

    std::cout << "Server component listening on port " << m_port << std::endl;

    std::vector<struct epoll_event> events(MAX_EVENTS);

    while (true) {
        int n = m_epoll.wait(events.data(), MAX_EVENTS, -1);
        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == m_listenerSocket.fd()) {
                handleNewConnection();
            } else {
                int client_fd = events[i].data.fd;
                // We still use a thread pool to handle the initial I/O event
                m_threadPool.enqueue([this, client_fd] {
                    handleClientData(client_fd);
                });
            }
        }
    }
}

void Server::handleNewConnection() {
    while (true) {
        Socket client_socket = m_listenerSocket.accept();
        if (client_socket.fd() == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                perror("accept");
                break;
            }
        }
        client_socket.setNonBlocking();
        m_epoll.add(client_socket.fd(), EPOLLIN | EPOLLET | EPOLLONESHOT);
        client_socket.release();
    }
}

void Server::handleClientData(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        // The RaftNode's handler will do the heavy lifting and is responsible for closing the fd.
        if (m_requestHandler) {
            m_requestHandler(client_fd, std::string(buffer));
        }
    } else {
        // If there's an error or disconnect on initial read, just close.
        m_epoll.remove(client_fd);
        close(client_fd);
    }
}