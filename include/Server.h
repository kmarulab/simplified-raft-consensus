#pragma once

#include "Socket.h"
#include "ThreadPool.h"
#include "Epoll.h"
#include <cstdint>
#include <functional> 

class Server {
public:
    Server(uint16_t port, std::function<void(int, const std::string&)> handler);
    void run();

private:
    void handleNewConnection();
    void handleClientData(int client_fd);

    uint16_t m_port;
    Socket m_listenerSocket;
    ThreadPool m_threadPool;
    Epoll m_epoll;
    
    std::function<void(int, const std::string&)> m_requestHandler;
};