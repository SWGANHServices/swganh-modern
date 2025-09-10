# src/network/CMakeLists.txt

set(NETWORK_SOURCES
    udp_server.cpp
    udp_server.hpp
)

add_library(swganh_network STATIC ${NETWORK_SOURCES})

target_include_directories(swganh_network PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(swganh_network PUBLIC
    swganh_core
    ${Boost_LIBRARIES}
    pthread
)

target_compile_features(swganh_network PUBLIC cxx_std_20)