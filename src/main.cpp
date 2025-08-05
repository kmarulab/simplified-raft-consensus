#include "RaftNode.h"
#include "third_party/nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <map>
#include <string>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <node_id>" << std::endl;
        return 1;
    }

    int nodeId = std::stoi(argv[1]);

    try {
        std::ifstream config_file("config.json");
        if (!config_file.is_open()) {
            throw std::runtime_error("Could not open config.json");
        }
        json config = json::parse(config_file);

        std::map<int, std::string> peers;
        for (const auto& peer : config["peers"]) {
            peers[peer["id"].get<int>()] = peer["address"].get<std::string>();
        }

        if (peers.find(nodeId) == peers.end()) {
            throw std::runtime_error("Node ID not found in config file.");
        }

        RaftNode node(nodeId, peers);
        node.run();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}