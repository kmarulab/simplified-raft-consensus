#include "KeyValueStore.h"
#include <shared_mutex> // For std::unique_lock, std::shared_lock
#include <mutex>

void KeyValueStore::set(const std::string& key, const std::string& value) {
    // Acquire a unique lock for writing. No other thread can read or write.
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_map[key] = value;
}

std::optional<std::string> KeyValueStore::get(const std::string& key) {
    // Acquire a shared lock for reading. Other threads can also read concurrently.
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_map.find(key);
    if (it != m_map.end()) {
        return it->second;
    }
    return std::nullopt; // C++17 way to return an empty optional
}

void KeyValueStore::del(const std::string& key) {
    // Acquire a unique lock for writing.
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_map.erase(key);
}