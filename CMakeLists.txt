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
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/network
        ${PROJECT_SOURCE_DIR}/src/traffic
        ${PROJECT_SOURCE_DIR}/src/metrics
        ${PROJECT_SOURCE_DIR}/src/config
        ${PROJECT_SOURCE_DIR}/src/server
        ${PROJECT_SOURCE_DIR}/src/client
)

# Source files
file(GLOB_RECURSE KUT_SOURCES
        src/main.cpp
        src/network/*.cpp
        src/traffic/*.cpp
        src/traffic/*/*.cpp
        src/traffic/*/*/*.cpp
        src/metrics/*.cpp
        src/config/*.cpp
        src/server/*.cpp
        src/client/*.cpp
)

# Main executable
add_executable(kut ${KUT_SOURCES}
        src/network/inetwork_server.h
        src/metrics/port_traffic_monitor.h
        src/metrics/port_traffic_monitor.cpp
        src/metrics/cpu_usage_monitor.cpp
        src/metrics/cpu_usage_monitor.h
        src/metrics/metric_store.h
        src/utils/pretty_printer.cpp
        src/utils/pretty_printer.h
        src/utils/csv_writer.cpp
        src/utils/csv_writer.h
        src/metrics/imetric_collector.h
        src/metrics/bytes_transferred_collector.cpp
        src/metrics/bytes_transferred_collector.h
        src/metrics/throughput_collector.cpp
        src/metrics/throughput_collector.h
        src/metrics/total_cpu_collector.cpp
        src/metrics/total_cpu_collector.h
        src/metrics/self_cpu_collector.cpp
        src/metrics/self_cpu_collector.h
        src/metrics/metric_collector_factory.cpp
        src/metrics/metric_collector_factory.h
        src/config/measurement_config.h
        src/network/tcp_control_channel.cpp
        src/network/tcp_control_channel.h
)

# Link yaml-cpp
target_link_libraries(kut PRIVATE yaml-cpp)

# --- begin necessary libpcap linkage ---
find_library(PCAP_LIB pcap)
if (NOT PCAP_LIB)
    message(FATAL_ERROR "libpcap not found; please install it (e.g., brew install libpcap).")
endif()
target_link_libraries(kut PRIVATE ${PCAP_LIB})
# --- end necessary libpcap linkage ---

# Compile definitions for ASIO
target_compile_definitions(kut PRIVATE ASIO_STANDALONE ASIO_NO_DEPRECATED)
