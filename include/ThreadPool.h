#pragma once

#include "TSQueue.h"
#include <vector>
#include <thread>
#include <functional>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    void enqueue(std::function<void()> task);
    void shutdown();

private:
    void worker_thread();

    std::vector<std::thread> m_workers;
    TSQueue<std::function<void()>> m_tasks;
    std::atomic<bool> m_stop;
};