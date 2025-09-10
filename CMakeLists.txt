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
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Build options
option(ENABLE_TESTING "Enable unit tests" ON)
option(ENABLE_PROFILING "Enable performance profiling" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(BUILD_PLUGINS "Build plugin system" ON)
option(BUILD_TOOLS "Build development tools" ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem thread program_options)
find_package(PostgreSQL REQUIRED)

# Find Lua
find_package(Lua REQUIRED)
if(NOT LUA_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LUA REQUIRED lua5.4)
endif()

# Find Redis client (hiredis)
pkg_check_modules(HIREDIS hiredis)
if(NOT HIREDIS_FOUND)
    message(WARNING "hiredis not found, Redis support will be limited")
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${Boost_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIRS}
)

# Link directories
link_directories(
    ${Boost_LIBRARY_DIRS}
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-fconcepts)
endif()

# AddressSanitizer support
if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

# Create output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add subdirectories
add_subdirectory(src/core)
add_subdirectory(src/network)
add_subdirectory(src/database)
add_subdirectory(src/servers)

if(BUILD_PLUGINS)
    add_subdirectory(src/plugins)
endif()

if(BUILD_TOOLS)
    add_subdirectory(src/tools)
endif()

# Testing
if(ENABLE_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS login_server zone_server
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY data/config/ DESTINATION share/swganh/config)
install(DIRECTORY data/sql/ DESTINATION share/swganh/sql)

# Print configuration summary
message(STATUS "SWG:ANH Modern Configuration Summary:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Boost Version: ${Boost_VERSION}")
message(STATUS "  PostgreSQL: ${PostgreSQL_FOUND}")
message(STATUS "  Lua Version: ${LUA_VERSION_STRING}")
message(STATUS "  Testing: ${ENABLE_TESTING}")
message(STATUS "  Profiling: ${ENABLE_PROFILING}")
message(STATUS "  AddressSanitizer: ${ENABLE_ASAN}")
message(STATUS "  Plugins: ${BUILD_PLUGINS}")
message(STATUS "  Tools: ${BUILD_TOOLS}")