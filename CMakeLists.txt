# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(HighPerfTCPServer CXX)

# Set the C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the threads library, which is required for std::thread
find_package(Threads REQUIRED)

# Add the 'include' directory to the include paths
include_directories(include)

# Add the executable target
add_executable(
    server
    src/main.cpp
    src/Server.cpp
    src/ThreadPool.cpp
    src/Socket.cpp
    src/Epoll.cpp
    src/KeyValueStore.cpp
    src/RaftNode.cpp
)

# Link the threads library to our server executable
target_link_libraries(server PRIVATE Threads::Threads)

# Optional: Add compiler flags for optimization and debugging
target_compile_options(server PRIVATE -O2 -g)