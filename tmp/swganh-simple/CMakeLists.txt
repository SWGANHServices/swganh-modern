# File: CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(swganh-simple VERSION 1.0.0)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

# Core library with config and account management
add_library(swganh_core STATIC
    src/core/logger.cpp
    src/core/account_manager.cpp
)
target_link_libraries(swganh_core Threads::Threads)

# Network library
add_library(swganh_network STATIC
    src/network/udp_server.cpp
)
target_link_libraries(swganh_network 
    swganh_core
    Boost::system 
    Threads::Threads
)

# Login server library
add_library(swganh_login STATIC
    src/servers/login/swg_protocol.cpp
)
target_link_libraries(swganh_login
    swganh_core
    swganh_network
)

# Login server executable
add_executable(login_server 
    src/servers/login/main.cpp
)
target_link_libraries(login_server 
    swganh_login
    swganh_network
    swganh_core
    Boost::system
    Threads::Threads
)

# Output directory
set_target_properties(login_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)