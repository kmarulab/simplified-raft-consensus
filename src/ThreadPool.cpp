#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads) : m_stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        m_workers.emplace_back(&ThreadPool::worker_thread, this);
    }
}

void ThreadPool::worker_thread() {
    while (true) {
        std::function<void()> task;
        if (!m_tasks.pop(task)) {
            // Queue is empty and shutdown has been called
            break;
        }
        task();
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    m_tasks.push(std::move(task));
}

void ThreadPool::shutdown() {
    m_stop = true;
    m_tasks.shutdown();
    for (std::thread& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

ThreadPool::~ThreadPool() {
    if (!m_stop) {
        shutdown();
    }
}