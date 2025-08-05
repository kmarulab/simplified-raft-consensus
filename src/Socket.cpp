#include "Socket.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <utility>

Socket::Socket() : m_fd(-1) {}

Socket::Socket(int fd) : m_fd(fd) {}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept : m_fd(other.m_fd) {
    other.m_fd = -1; // Invalidate the other socket
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();
        m_fd = other.m_fd;
        other.m_fd = -1;
    }
    return *this;
}

void Socket::create() {
    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_fd < 0) {
        throw std::runtime_error("Failed to create socket");
    }
}

void Socket::bind(uint16_t port) {
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (::bind(m_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("Failed to bind socket");
    }
}

void Socket::listen(int backlog) {
    if (::listen(m_fd, backlog) < 0) {
        throw std::runtime_error("Failed to listen on socket");
    }
}

Socket Socket::accept() {
    int client_fd = ::accept(m_fd, nullptr, nullptr);
    if (client_fd < 0) {
        // This can happen in non-blocking mode, not a fatal error
        return Socket(-1); 
    }
    return Socket(client_fd);
}

void Socket::connect(const std::string& host, uint16_t port) {
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr);

    if (::connect(m_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        throw std::runtime_error("Failed to connect");
    }
}

void Socket::setNonBlocking() {
    int flags = fcntl(m_fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("Failed to get socket flags");
    }
    if (fcntl(m_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("Failed to set non-blocking");
    }
}

void Socket::close() {
    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }
}

int Socket::fd() const {
    return m_fd;
}

int Socket::release() {
    int temp_fd = m_fd;
    m_fd = -1; // This object no longer owns the file descriptor
    return temp_fd;
}