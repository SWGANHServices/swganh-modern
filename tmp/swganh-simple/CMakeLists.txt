cmake_minimum_required(VERSION 3.16)
project(swganh-test)

set(CMAKE_CXX_STANDARD 20)

add_executable(login_server src/servers/login/main.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

message(STATUS "Simple build configuration complete")