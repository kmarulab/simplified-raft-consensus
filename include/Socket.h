#pragma once

#include <string>
#include <cstdint>

class Socket {
public:
    Socket();
    explicit Socket(int fd);
    ~Socket();

    // Disable copy semantics
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    // Enable move semantics
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    void create();
    void bind(uint16_t port);
    void listen(int backlog = 128);
    Socket accept();
    void connect(const std::string& host, uint16_t port);
    
    void setNonBlocking();
    void close();
    int release();
    
    int fd() const;

private:
    int m_fd;
};