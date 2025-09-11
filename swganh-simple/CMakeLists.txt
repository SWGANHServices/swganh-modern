cmake_minimum_required(VERSION 3.16)
project(swganh-modern VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find required packages
find_package(Boost REQUIRED COMPONENTS system filesystem thread)

# Include directories
include_directories(src)

# Core library sources
set(CORE_SOURCES
    src/core/logger.cpp
)

# Network library sources  
set(NETWORK_SOURCES
    src/network/udp_server.cpp
)

# Create libraries
add_library(swganh_core STATIC ${CORE_SOURCES})
add_library(swganh_network STATIC ${NETWORK_SOURCES})

# Link libraries
target_link_libraries(swganh_network 
    swganh_core 
    ${Boost_LIBRARIES} 
    pthread
)

# Login server executable
add_executable(login_server
    src/servers/login/main.cpp
)

target_link_libraries(login_server
    swganh_core
    swganh_network
    ${Boost_LIBRARIES}
    pthread
)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

message(STATUS "Build configuration complete")