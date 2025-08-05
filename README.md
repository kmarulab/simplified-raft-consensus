# High-Performance, Fault-Tolerant Key-Value Store with Raft Leader Election

This project is a high-performance, distributed, in-memory key-value store built from scratch in C++17 and google's Gemini. It demonstrates key principles of distributed systems by implementing a simplified version of the **Raft consensus algorithm** for leader election and fault tolerance. The cluster can automatically detect leader failure and elect a new leader, ensuring high availability.

The underlying networking layer is built on a non-blocking I/O event loop using `epoll` on Linux, coupled with a thread pool to process client and peer requests, making it capable of handling a high degree of concurrency.

This project is the culmination of a guided development process, evolving from a simple TCP echo server to a single-node key-value store, and finally to this resilient, distributed cluster.

## Key Features

- **High-Performance Networking:** Non-blocking I/O architecture using Linux `epoll` for efficient event handling.

- **Multi-Threaded Processing:** A fixed-size thread pool to handle client and peer requests concurrently.

- **Distributed Consensus:** Simplified Raft-based leader election for high availability.

- **Automatic Failover:** The cluster can withstand the failure of its leader and elect a new one without manual intervention.

- **In-Memory Key-Value Store:** A fast, stateful application layer providing `SET`, `GET`, and `DELETE` functionality.

- **Modern C++:** Built with C++17, leveraging modern features like smart pointers, RAII, `std::thread`, `std::mutex`, `std::atomic`, and lambdas for robust and clean code.

## Architecture

The system is composed of three primary layers:

1. **Networking Layer:** A highly efficient, non-blocking TCP server built with the Linux `epoll` API. This layer is responsible for accepting connections and reading raw data from sockets without ever blocking the main event loop.

2. **Concurrency Layer:** A thread pool of worker threads. The main I/O thread dispatches incoming requests to this pool for processing. This separates the fast I/O handling from potentially slower application logic.

3. **Consensus & Application Layer:** This is managed by the `RaftNode` class. Each instance of the server is a `RaftNode` that encapsulates:

    - **Raft State Machine:** Manages the node's state (Follower, Candidate, or Leader), term numbers, and election timers.

    - **RPC Mechanism:** Nodes communicate with each other using JSON-based RPCs for heartbeats and voting.

    - **Application Logic:** The Leader node processes client requests for the `KeyValueStore`. Followers will reject client write requests and redirect them to the leader.

## Getting Started

### Prerequisites

- A C++17 compliant compiler (e.g., `g++` 7 or later)

- `CMake` (version 3.10 or higher)

- A Linux-based operating system (for `epoll`)

### Build Instructions

1. Clone the repository to your local machine.
    ```Bash
    git clone https://github.com/kmarulab/simplified-raft-consensus
    ```
    
2. Create and navigate to the build directory:

    ```Bash
    mkdir build && cd build
    ```

3. Run CMake to configure the project:

    ```Bash
    cmake ..
    ```

4. Compile the source code:

    ```Bash
    make
    ```

    An executable named `server` will be created in the `build` directory.

## Configuration

The cluster's configuration is defined in the `config.json` file located in the project's root directory. You must create this file before running the server. It lists all the peers in the cluster with their unique ID and network address.

**Example `config.json` for a 3-node cluster:**

```JSON
{
  "peers": [
    {
      "id": 0,
      "address": "127.0.0.1:8080"
    },
    {
      "id": 1,
      "address": "127.0.0.1:8081"
    },
    {
      "id": 2,
      "address": "127.0.0.1:8082"
    }
  ]
}
```

## Running the Cluster

To run the cluster, you need to launch one server process for each peer defined in `config.json`. Each process must be started in its own terminal window.

Navigate to your project's root directory in three separate terminals.

- **In Terminal 1:**

    ```Bash
    ./build/server 0
    ```

- **In Terminal 2:**

    ```Bash
    ./build/server 1
    ```

- **In Terminal 3:**

    ```Bash
    ./build/server 2
    ```

As the nodes start, you will see them initialize as Followers. After a short, randomized timeout, one will become a Candidate, start an election, and be elected Leader. The new Leader will begin sending heartbeats to the other nodes.

## How to Use and Test the System

### Testing Fault Tolerance

This is the most exciting part.

1. Observe the terminal outputs to identify which node has become the **LEADER**.

2. Kill that leader's process by pressing `Ctrl+C` in its terminal.

3. Watch the output of the other two Follower nodes. Within milliseconds, you will see one of them time out, become a Candidate, and get elected as the new Leader for the next term. The cluster has automatically recovered.

### Interacting with the Key-Value Store

You must send your requests to the **current Leader**. If you send a request to a Follower, it will be rejected with an error message indicating who the leader is.

You can use `netcat` (`nc`) from a new terminal to interact with the service. The connection is closed after each request.

**Example Session:**

1. Assume Node 1 (running on port 8081) is the current Leader.

2. **Set a value:**

    ```Bash
    echo 'SET user:100 kmarulab' | nc localhost 8081
    ```

    **Response:** `{"status":"OK"}`

3. **Get a value:**

    ```Bash
    echo 'GET user:100' | nc localhost 8081
    ```

    **Response:** `{"value":"kmarulab"}`

4. **Try to write to a Follower** (e.g., Node 2 on port 8082):

    ```Bash
    echo 'SET user:100 test' | nc localhost 8082
    ```

    **Response:** `{"error":"NOT_LEADER","leader_hint":"127.0.0.1:8081"}`
