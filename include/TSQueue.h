#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template <typename T>
class TSQueue {
public:
    TSQueue() = default;
    TSQueue(const TSQueue&) = delete;
    TSQueue& operator=(const TSQueue&) = delete;

    void push(T item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(item));
        m_cond.notify_one();
    }

    // Blocking pop
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this] { return !m_queue.empty() || m_stop; });

        if (m_stop && m_queue.empty()) {
            return false;
        }

        item = std::move(m_queue.front());
        m_queue.pop();
        return true;
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
        m_cond.notify_all(); // Wake up all waiting threads
    }

private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    bool m_stop = false;
};