cmake_minimum_required(VERSION 3.20)
project(kut LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(YAML_CPP_BUILD_TESTS OFF CACHE INTERNAL "")
set(YAML_CPP_BUILD_TOOLS OFF CACHE INTERNAL "")

# External dependencies
add_subdirectory(external/yaml-cpp)

# ASIO (header-only)
include_directories(
        external/asio/asio/include
)

# Include project headers
include_directories(
        ${PROJECT_SOURCE_DIR}/src/
        ${PROJECT_SOURCE_DIR}/src/network
        ${PROJECT_SOURCE_DIR}/src/traffic
        ${PROJECT_SOURCE_DIR}/src/metrics
        ${PROJECT_SOURCE_DIR}/src/config
        ${PROJECT_SOURCE_DIR}/src/utils
)

# Source files
file(GLOB_RECURSE KUT_SOURCES
        src/*.cpp
)

# Main executable
add_executable(kut ${KUT_SOURCES})

# Link yaml-cpp
target_link_libraries(kut PRIVATE yaml-cpp)

# Compile definitions for ASIO
target_compile_definitions(kut PRIVATE ASIO_STANDALONE ASIO_NO_DEPRECATED)
