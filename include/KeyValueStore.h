#pragma once

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <optional>

class KeyValueStore {
public:
    KeyValueStore() = default;

    // Sets a value for a given key.
    void set(const std::string& key, const std::string& value);

    // Gets a value for a given key. Returns an empty optional if the key is not found.
    std::optional<std::string> get(const std::string& key);

    // Deletes a key-value pair.
    void del(const std::string& key);

private:
    // The underlying hash map that stores the data.
    std::unordered_map<std::string, std::string> m_map;
    
    // A reader-writer mutex for thread-safe access.
    mutable std::shared_mutex m_mutex;
};