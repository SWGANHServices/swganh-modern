# src/core/CMakeLists.txt

set(CORE_SOURCES
    types.hpp
    logger.cpp
    logger.hpp
    config.cpp
    config.hpp
    service.cpp
    service.hpp
    timer.cpp
    timer.hpp
)

add_library(swganh_core STATIC ${CORE_SOURCES})

target_include_directories(swganh_core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(swganh_core PUBLIC
    ${Boost_LIBRARIES}
    pthread
)

target_compile_features(swganh_core PUBLIC cxx_std_20)