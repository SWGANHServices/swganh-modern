cmake_minimum_required(VERSION 3.16)
project(swganh-modern VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add subdirectories
add_subdirectory(src/core)
add_subdirectory(src/plugins)
add_subdirectory(src/servers)
add_subdirectory(tests)

# Enable testing
enable_testing()
