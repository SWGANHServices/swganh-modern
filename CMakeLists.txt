# Root CMakeLists.txt (UPDATE the existing one)
cmake_minimum_required(VERSION 3.16)
project(swganh-modern VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build configuration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build types
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4 /WX- /permissive-)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7+
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

# Platform-specific settings
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Global include directory
include_directories(${PROJECT_SOURCE_DIR}/src)

# Add subdirectories
add_subdirectory(src/core)
add_subdirectory(src/servers)

# Testing
option(ENABLE_TESTING "Enable testing" ON)
if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS login_server
        RUNTIME DESTINATION bin
        COMPONENT runtime)

install(DIRECTORY data/
        DESTINATION share/swganh-modern/data
        COMPONENT data)

# Package configuration
set(CPACK_PACKAGE_NAME "swganh-modern")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SWG:ANH Modern Server")
set(CPACK_PACKAGE_VENDOR "SWGANHServices")

include(CPack)

# Print configuration summary
message(STATUS "=== SWG:ANH Modern Build Configuration ===")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Testing: ${ENABLE_TESTING}")
message(STATUS "=========================================")

# src/core/CMakeLists.txt
add_library(swganh-core
    network/soe_protocol.cpp
    network/soe_protocol.hpp
    network/udp_server.hpp
)

# Core library includes
target_include_directories(swganh-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Core library dependencies
target_link_libraries(swganh-core
    Threads::Threads
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(swganh-core ws2_32)
endif()

# Export symbols for shared library
if(BUILD_SHARED_LIBS)
    target_compile_definitions(swganh-core PRIVATE SWGANH_CORE_EXPORTS)
endif()

# src/servers/CMakeLists.txt
add_subdirectory(login)

# src/servers/login/CMakeLists.txt
add_executable(login_server
    login_server.hpp
    login_server.cpp
    main.cpp
)

# Link against core library
target_link_libraries(login_server
    swganh-core
    Threads::Threads
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(login_server ws2_32)
endif()

# Set output directory
set_target_properties(login_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy configuration files to build directory
configure_file(
    ${PROJECT_SOURCE_DIR}/data/config/server.json
    ${CMAKE_BINARY_DIR}/bin/config/server.json
    COPYONLY
)

# tests/CMakeLists.txt
# Basic test framework (we'll expand this later)
add_executable(test_soe_protocol
    test_soe_protocol.cpp
)

target_link_libraries(test_soe_protocol
    swganh-core
    Threads::Threads
)

# Add test
add_test(NAME SOEProtocolTests COMMAND test_soe_protocol)

# Development Dockerfile: docker/dev/Dockerfile.dev
FROM ubuntu:22.04

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    ninja-build \
    clang-14 \
    clang-tools-14 \
    gdb \
    valgrind \
    cppcheck \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set up C++ environment
ENV CC=clang-14
ENV CXX=clang++-14

# Create workspace
WORKDIR /workspace

# Copy project files
COPY . .

# Create build directory
RUN mkdir -p build

# Development entry point
CMD ["bash"]

# docker/dev/docker-compose.dev.yml (UPDATE existing)
version: '3.8'

services:
  swganh-dev:
    build:
      context: ../../
      dockerfile: docker/dev/Dockerfile.dev
    volumes:
      - ../../:/workspace
      - build-cache:/workspace/build
    ports:
      - "44453:44453"  # Login server
      - "44463:44463"  # Zone server (future)
      - "8080:8080"    # Admin interface (future)
    depends_on:
      - postgres
      - redis
    environment:
      - BUILD_TYPE=Debug
      - POSTGRES_HOST=postgres
      - REDIS_HOST=redis
    working_dir: /workspace
    command: >
      bash -c "
        cd build && 
        cmake .. -DCMAKE_BUILD_TYPE=Debug -GNinja &&
        ninja &&
        echo 'Build complete. Starting login server...' &&
        ./bin/login_server --address 0.0.0.0 --port 44453
      "
  
  postgres:
    image: postgres:14
    environment:
      POSTGRES_DB: swganh_dev
      POSTGRES_USER: swganh
      POSTGRES_PASSWORD: devpassword
    ports:
      - "5432:5432"
    volumes:
      - postgres-data:/var/lib/postgresql/data
      - ../../data/sql:/docker-entrypoint-initdb.d
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U swganh"]
      interval: 5s
      timeout: 5s
      retries: 5
  
  redis:
    image: redis:7-alpine
    ports:
      - "6379:6379"
    command: redis-server --appendonly yes
    volumes:
      - redis-data:/data
    healthcheck:
      test: ["CMD", "redis-cli", "ping"]
      interval: 5s
      timeout: 3s
      retries: 5

  # Database administration
  pgadmin:
    image: dpage/pgadmin4
    environment:
      PGADMIN_DEFAULT_EMAIL: admin@swganh.dev
      PGADMIN_DEFAULT_PASSWORD: admin
    ports:
      - "5050:80"
    depends_on:
      - postgres

volumes:
  postgres-data:
  redis-data:
  build-cache:

# Basic test file: tests/test_soe_protocol.cpp
#include "../src/core/network/soe_protocol.hpp"
#include <iostream>
#include <cassert>

using namespace swganh::network;

void TestSOEPacketBasics() {
    std::cout << "Testing SOE packet basics..." << std::endl;
    
    // Test packet creation
    SOEPacket packet(SOE_SESSION_REQUEST);
    assert(packet.GetOpcode() == SOE_SESSION_REQUEST);
    
    // Test writing data
    packet.WriteUInt32(12345);
    packet.WriteString("test");
    
    // Test reading data
    packet.ReadUInt16(); // Skip opcode
    uint32_t value = packet.ReadUInt32();
    assert(value == 12345);
    
    std::string str = packet.ReadString();
    assert(str == "test");
    
    std::cout << "SOE packet basics: PASSED" << std::endl;
}

void TestCRCCalculation() {
    std::cout << "Testing CRC calculation..." << std::endl;
    
    const uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    uint16_t crc = SOEProtocolHandler::CalculateChecksum(test_data, 4, SOE_CRC_SEED);
    
    // CRC should be deterministic
    uint16_t crc2 = SOEProtocolHandler::CalculateChecksum(test_data, 4, SOE_CRC_SEED);
    assert(crc == crc2);
    
    std::cout << "CRC calculation: PASSED" << std::endl;
}

void TestBasicSOEHandler() {
    std::cout << "Testing BasicSOEHandler..." << std::endl;
    
    BasicSOEHandler handler;
    
    // Test session creation
    handler.CreateSession("127.0.0.1", 12345);
    
    auto* session = handler.GetSessionByEndpoint("127.0.0.1", 12345);
    assert(session != nullptr);
    assert(session->remote_address == "127.0.0.1");
    assert(session->remote_port == 12345);
    
    std::cout << "BasicSOEHandler: PASSED" << std::endl;
}

int main() {
    std::cout << "Running SOE Protocol Tests..." << std::endl;
    
    try {
        TestSOEPacketBasics();
        TestCRCCalculation();
        TestBasicSOEHandler();
        
        std::cout << "All tests PASSED!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cout << "Test FAILED: " << e.what() << std::endl;
        return 1;
    }
}

# Build script: scripts/build.sh
#!/bin/bash

set -e

# Configuration
BUILD_TYPE=${BUILD_TYPE:-Debug}
BUILD_DIR=${BUILD_DIR:-build}
JOBS=${JOBS:-$(nproc)}

echo "=== SWG:ANH Modern Build Script ==="
echo "Build Type: $BUILD_TYPE"
echo "Build Directory: $BUILD_DIR"
echo "Jobs: $JOBS"
echo "=================================="

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "Configuring build..."
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DENABLE_TESTING=ON

# Build
echo "Building..."
cmake --build . --parallel "$JOBS"

# Test
echo "Running tests..."
ctest --output-on-failure

echo "Build completed successfully!"
echo "Executables are in: $BUILD_DIR/bin/"

# Development script: scripts/dev.sh
#!/bin/bash

echo "Starting SWG:ANH Modern Development Environment..."

# Start all services
docker-compose -f docker/dev/docker-compose.dev.yml up --build

# Quick start script: scripts/start.sh
#!/bin/bash

# Quick development start
echo "Quick starting SWG:ANH Modern..."

# Build and run
./scripts/build.sh
echo "Starting login server..."
./build/bin/login_server --address 0.0.0.0 --port 44453
