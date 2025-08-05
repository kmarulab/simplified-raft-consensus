#pragma once

#include "Server.h"
#include "KeyValueStore.h"
#include "third_party/nlohmann/json.hpp" 
#include <thread>
#include <atomic>
#include <chrono>
#include <map>
#include <optional>
#include <mutex>

using json = nlohmann::json;

enum class NodeState { Follower, Candidate, Leader };

class RaftNode {
public:
    RaftNode(int nodeId, const std::map<int, std::string>& peers);
    ~RaftNode();
    void run();

private:
    // Raft logic state machine
    void raftLogicLoop();

    // State transition methods
    void becomeFollower(uint64_t term);
    void becomeCandidate();
    void becomeLeader();

    // RPC handling
    void handleRequest(int client_fd, const std::string& request);
    json handleRpc(const json& rpc);
    json handleClientRequest(const std::string& request);

    // RPC sending
    void sendHeartbeats();
    void startElection();
    
    // Timer utility
    void resetElectionTimer();
    
    // Member variables
    int m_nodeId;
    std::map<int, std::string> m_peers;
    Server m_server;
    KeyValueStore m_store;
    std::thread m_raftLogicThread;
    std::atomic<bool> m_stopFlag;

    // Raft Persistent State
    std::atomic<uint64_t> m_currentTerm;
    std::atomic<int> m_votedFor; 

    // Raft Volatile State
    NodeState m_state;
    std::optional<int> m_currentLeaderId;
    std::mutex m_raftMutex;

    // Timers
    std::chrono::steady_clock::time_point m_lastHeartbeatTime;
    std::chrono::milliseconds m_electionTimeout;
};