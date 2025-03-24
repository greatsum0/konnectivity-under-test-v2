cmake_minimum_required(VERSION 3.30)
project(kut)

set(CMAKE_CXX_STANDARD 20)
include(FetchContent)

# get the yaml-cpp from GitHub directly
FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG 0.8.0 # stable version at the time of writing
)

# Boost.Asio standalone (header-only)
FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-30-2
)

FetchContent_MakeAvailable(yaml-cpp asio)

include_directories(
        ${asio_SOURCE_DIR}/asio/include
        ${PROJECT_SOURCE_DIR}/src/network
        ${PROJECT_SOURCE_DIR}/src/traffic
        ${PROJECT_SOURCE_DIR}/src/metrics
        ${PROJECT_SOURCE_DIR}/src/config
        ${PROJECT_SOURCE_DIR}/src/utils
)


add_executable(kut
        src/main.cpp
        src/config/yaml_parser.cpp
        src/network/tcp_server.h
        src/network/tcp_server.cpp
        src/network/udp_server.cpp
        src/network/udp_server.h
        src/network/tcp_client.h
        src/network/tcp_client.cpp
        src/network/udp_client.h
        src/network/udp_client.cpp
        src/metrics/metric_collector.h
        src/metrics/metric_collector.cpp
        src/utils/metric_utils.cpp
        src/utils/metric_utils.h
        src/config/arg_parser.cpp
        src/config/arg_parser.h
        src/traffic/traffic_pattern.h
        src/traffic/constant/constant_traffic_pattern.cpp
        src/traffic/constant/constant_traffic_pattern.h
        src/network/inetwork_client.h
        src/network/iserver.h
        src/config/client_context.h
        src/config/server_context.h
        src/traffic/traffic_pattern_factory.cpp
        src/traffic/traffic_pattern_factory.h
        src/traffic/burst/burst_traffic_pattern.cpp
        src/traffic/burst/burst_traffic_pattern.h
        src/traffic/burst/pure_burst_traffic_pattern.cpp
        src/traffic/burst/pure_burst_traffic_pattern.h
        src/traffic/burst/abstract_burst_pattern.cpp
        src/traffic/burst/abstract_burst_pattern.h
        src/traffic/burst/random_burst_traffic_pattern.cpp
        src/traffic/burst/random_burst_traffic_pattern.h
        src/traffic/sinusoidal/sinusoidal_traffic_pattern.cpp
        src/traffic/sinusoidal/sinusoidal_traffic_pattern.h
        src/traffic/random/random_traffic_pattern.cpp
        src/traffic/random/random_traffic_pattern.h
        src/traffic/ramp/abstract_ramp_pattern.cpp
        src/traffic/ramp/abstract_ramp_pattern.h
        src/traffic/ramp/ramp_up_traffic_pattern.cpp
        src/traffic/ramp/ramp_up_traffic_pattern.h
        src/traffic/ramp/ramp_down_traffic_pattern.cpp
        src/traffic/ramp/ramp_down_traffic_pattern.h
        src/metrics/metric_data_manager.cpp
        src/metrics/metric_data_manager.h)

# Only yaml-cpp needs linking
target_link_libraries(kut PRIVATE yaml-cpp::yaml-cpp)

# For standalone ASIO, these definitions are required
target_compile_definitions(kut PRIVATE ASIO_STANDALONE ASIO_NO_DEPRECATED)