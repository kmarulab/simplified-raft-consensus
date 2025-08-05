#include "RaftNode.h"
#include <iostream>
#include <random>
#include <thread>
#include <sstream>
#include <unistd.h>

using json = nlohmann::json;

// Helper function to split address:port string
std::pair<std::string, uint16_t> parseAddress(const std::string& address) {
    size_t colon_pos = address.find(':');
    if (colon_pos == std::string::npos) {
        throw std::runtime_error("Invalid address format: " + address);
    }
    std::string ip = address.substr(0, colon_pos);
    uint16_t port = std::stoi(address.substr(colon_pos + 1));
    return {ip, port};
}

// Helper function to send an RPC message to a peer
json sendRpc(const std::string& address, const json& request) {
    try {
        auto [ip, port] = parseAddress(address);
        Socket peer_socket;
        peer_socket.create();
        peer_socket.connect(ip, port);

        std::string request_str = request.dump();
        [[maybe_unused]] ssize_t bytes_written = write(peer_socket.fd(), request_str.c_str(), request_str.length());
        
        char buffer[4096];
        ssize_t bytes_read = read(peer_socket.fd(), buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            return json::parse(buffer);
        }
    } catch (const std::exception& e) {
        // Peer might be down, this is expected.
    }
    return json::object(); // Return empty json on failure
}

RaftNode::RaftNode(int nodeId, const std::map<int, std::string>& peers)
    : m_nodeId(nodeId), 
      m_peers(peers),
      m_server(parseAddress(peers.at(nodeId)).second, 
               [this](int fd, const std::string& req){ this->handleRequest(fd, req); }),
      m_stopFlag(false),
      m_currentTerm(0),
      m_votedFor(-1),
      m_state(NodeState::Follower) {
    
    std::cout << "Node " << m_nodeId << " starting up at " << peers.at(nodeId) << "..." << std::endl;
    resetElectionTimer();
}

RaftNode::~RaftNode() {
    m_stopFlag = true;
    if (m_raftLogicThread.joinable()) {
        m_raftLogicThread.join();
    }
}

void RaftNode::run() {
    m_raftLogicThread = std::thread(&RaftNode::raftLogicLoop, this);
    m_server.run(); // This blocks, so it should be last.
}

void RaftNode::resetElectionTimer() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(250, 500); // Randomized timeouts are key to Raft
    m_electionTimeout = std::chrono::milliseconds(distrib(gen));
    m_lastHeartbeatTime = std::chrono::steady_clock::now();
}

void RaftNode::raftLogicLoop() {
    while (!m_stopFlag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        std::unique_lock<std::mutex> lock(m_raftMutex);

        if (m_state == NodeState::Leader) {
             auto elapsed = std::chrono::steady_clock::now() - m_lastHeartbeatTime;
            if (elapsed > std::chrono::milliseconds(100)) { // Heartbeat interval
                lock.unlock();
                sendHeartbeats();
                lock.lock();
                m_lastHeartbeatTime = std::chrono::steady_clock::now();
            }
        } else { // Follower or Candidate
            auto elapsed = std::chrono::steady_clock::now() - m_lastHeartbeatTime;
            if (elapsed > m_electionTimeout) {
                std::cout << "Node " << m_nodeId << " (Term " << m_currentTerm << "): Election timeout, becoming candidate." << std::endl;
                becomeCandidate();
            }
        }
    }
}

void RaftNode::becomeFollower(uint64_t term) {
    m_state = NodeState::Follower;
    m_currentTerm = term;
    m_votedFor = -1;
    m_currentLeaderId.reset();
    std::cout << "Node " << m_nodeId << " became FOLLOWER for term " << m_currentTerm << "." << std::endl;
}

void RaftNode::becomeCandidate() {
    m_state = NodeState::Candidate;
    m_currentTerm++;
    m_votedFor = m_nodeId; // Vote for self
    m_currentLeaderId.reset();
    resetElectionTimer();
    // Unlock before sending RPCs to avoid deadlock
    m_raftMutex.unlock();
    startElection();
    m_raftMutex.lock();
}

void RaftNode::becomeLeader() {
    if (m_state != NodeState::Leader) {
        m_state = NodeState::Leader;
        m_currentLeaderId = m_nodeId;
        std::cout << "\n=================================================" << std::endl;
        std::cout << "Node " << m_nodeId << " became LEADER for term " << m_currentTerm << "!" << std::endl;
        std::cout << "=================================================\n" << std::endl;
        // Unlock before sending RPCs
        m_raftMutex.unlock();
        sendHeartbeats();
        m_raftMutex.lock();
    }
}

void RaftNode::startElection() {
    std::atomic<int> votesReceived = 1; // Vote for self
    json request = {
        {"type", "RequestVote"},
        {"term", m_currentTerm.load()},
        {"candidateId", m_nodeId}
    };

    std::cout << "Node " << m_nodeId << " starting election for term " << m_currentTerm << "." << std::endl;

    for (const auto& peer : m_peers) {
        if (peer.first == m_nodeId) continue;
        
        std::thread([this, peer, request, &votesReceived]() {
            json response = sendRpc(peer.second, request);
            if (!response.empty() && response.value("voteGranted", false)) {
                votesReceived++;
                std::lock_guard<std::mutex> lock(m_raftMutex);
                if (m_state == NodeState::Candidate && votesReceived > m_peers.size() / 2) {
                    becomeLeader();
                }
            } else if (!response.empty()) {
                std::lock_guard<std::mutex> lock(m_raftMutex);
                uint64_t responseTerm = response.value("term", 0);
                if (responseTerm > m_currentTerm) {
                    becomeFollower(responseTerm);
                }
            }
        }).detach();
    }
}

void RaftNode::sendHeartbeats() {
    json request = {
        {"type", "AppendEntries"},
        {"term", m_currentTerm.load()},
        {"leaderId", m_nodeId}
    };
    for (const auto& peer : m_peers) {
        if (peer.first == m_nodeId) continue;
        std::thread([this, peer, request]() {
            json response = sendRpc(peer.second, request);
            if (!response.empty()) {
                std::lock_guard<std::mutex> lock(m_raftMutex);
                uint64_t responseTerm = response.value("term", 0);
                if (responseTerm > m_currentTerm) {
                    becomeFollower(responseTerm);
                }
            }
        }).detach();
    }
}

void RaftNode::handleRequest(int client_fd, const std::string& request_str) {
    json response;
    if (request_str.empty()) {
        close(client_fd);
        return;
    }

    try {
        json request = json::parse(request_str);
        if (request.contains("type")) {
            response = handleRpc(request);
        } else {
            response = handleClientRequest(request_str);
        }
    } catch (json::parse_error& e) {
        response = handleClientRequest(request_str);
    }
    
    std::string response_str = response.dump();
    [[maybe_unused]] ssize_t bytes_written = write(client_fd, response_str.c_str(), response_str.length());
    close(client_fd);
}

json RaftNode::handleRpc(const json& rpc) {
    std::lock_guard<std::mutex> lock(m_raftMutex);
    uint64_t requestTerm = rpc.value("term", (uint64_t)0);

    if (requestTerm > m_currentTerm) {
        becomeFollower(requestTerm);
    }
    
    if (rpc["type"] == "RequestVote") {
        if (requestTerm < m_currentTerm) {
            return {{"term", m_currentTerm.load()}, {"voteGranted", false}};
        }
        bool voteGranted = (m_votedFor == -1 || m_votedFor == rpc["candidateId"].get<int>());
        if (voteGranted) {
            m_votedFor = rpc["candidateId"].get<int>();
            resetElectionTimer(); // Reset timer if we grant a vote
        }
        return {{"term", m_currentTerm.load()}, {"voteGranted", voteGranted}};

    } else if (rpc["type"] == "AppendEntries") {
        if (requestTerm < m_currentTerm) {
            return {{"term", m_currentTerm.load()}, {"success", false}};
        }
        if (m_state == NodeState::Candidate) {
             becomeFollower(requestTerm);
        }
        m_currentLeaderId = rpc["leaderId"].get<int>();
        resetElectionTimer(); 
        return {{"term", m_currentTerm.load()}, {"success", true}};
    }
    return {{"error", "Unknown RPC type"}};
}

json RaftNode::handleClientRequest(const std::string& request_str) {
    std::lock_guard<std::mutex> lock(m_raftMutex);
    if (m_state != NodeState::Leader) {
        json response;
        response["error"] = "NOT_LEADER";
        if (m_currentLeaderId.has_value()) {
            if (m_peers.count(*m_currentLeaderId)) {
                response["leader_hint"] = m_peers.at(*m_currentLeaderId);
            }
        }
        return response;
    }

    std::stringstream ss(request_str);
    std::string command, key, value;
    ss >> command >> key;
    if (ss.peek() == ' ') ss.ignore();
    std::getline(ss, value);

    if (command == "GET") {
        auto result = m_store.get(key);
        return {{"value", result.value_or("(nil)")}};
    } else if (command == "SET") {
        m_store.set(key, value);
        return {{"status", "OK"}};
    } else if (command == "DELETE") {
        m_store.del(key);
        return {{"status", "OK"}};
    }
    return {{"error", "Unknown command"}};
}