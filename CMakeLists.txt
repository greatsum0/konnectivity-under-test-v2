cmake_minimum_required(VERSION 3.30)
project(kut LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find Boost
find_package(Boost 1.70 REQUIRED COMPONENTS system thread)
if (NOT Boost_FOUND)
    message(FATAL_ERROR "Boost not found")
endif()

# Find yaml-cpp
find_package(yaml-cpp REQUIRED)
if (NOT yaml-cpp_FOUND)
    message(FATAL_ERROR "yaml-cpp not found")
endif ()

find_library(PCAP_LIB pcap)
if (NOT PCAP_LIB)
    message(FATAL_ERROR "libpcap not found; please install it (e.g., brew install libpcap).")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if (NOT Threads_FOUND)
    message(FATAL_ERROR "Threads not found")
endif ()

# Collect all the .cpp sources under src/
file(GLOB_RECURSE KUT_SOURCES
        ${PROJECT_SOURCE_DIR}/src/main.cpp
        #        # ------------------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/src/context/*.cpp
        #        # ------------------------------------------------------------------
        #        ${PROJECT_SOURCE_DIR}/src/controller/*.cpp
        #        # ------------------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/src/monitor/*.cpp
        #        ${PROJECT_SOURCE_DIR}/src/monitor/network/*.cpp
        #        ${PROJECT_SOURCE_DIR}/src/monitor/network/collector/*.cpp
        #        ${PROJECT_SOURCE_DIR}/src/monitor/system/*.cpp
        #        ${PROJECT_SOURCE_DIR}/src/monitor/system/collector/*.cpp
        #        # ------------------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/src/network/client/tcp/*.cpp
        ${PROJECT_SOURCE_DIR}/src/network/client/udp/*.cpp
        ${PROJECT_SOURCE_DIR}/src/network/server/tcp/*.cpp
        ${PROJECT_SOURCE_DIR}/src/network/server/udp/*.cpp
        #        # ------------------------------------------------------------------
        #        ${PROJECT_SOURCE_DIR}/src/parser/cli/*.cpp
        # ${PROJECT_SOURCE_DIR}/src/parser/yaml/*.cpp
        #        # ------------------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/src/pattern/*.cpp
        ${PROJECT_SOURCE_DIR}/src/pattern/burst/*.cpp
        ${PROJECT_SOURCE_DIR}/src/pattern/constant/*.cpp
        ${PROJECT_SOURCE_DIR}/src/pattern/ramp/*.cpp
        ${PROJECT_SOURCE_DIR}/src/pattern/sinusoidal/*.cpp
        #        # ------------------------------------------------------------------
        #        ${PROJECT_SOURCE_DIR}/src/session/client/*.cpp
        #        ${PROJECT_SOURCE_DIR}/src/session/server/*.cpp
        #        # ------------------------------------------------------------------
        #        ${PROJECT_SOURCE_DIR}/src/traffic/*.cpp
        #        # ------------------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/src/utils/*.cpp
        ${PROJECT_SOURCE_DIR}/src/utils/csv/*.cpp
        ${PROJECT_SOURCE_DIR}/src/utils/printer/*.cpp
)

# Define the executable
add_executable(${PROJECT_NAME} ${KUT_SOURCES})

# Pull in Boost headers
target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${Boost_INCLUDE_DIRS}
)

# Expose your header‐folders so you can #include "foo.h" by filename
target_include_directories(${PROJECT_NAME}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        #        # ------------------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/src/context
        #        # ------------------------------------------------------------------
        #        ${PROJECT_SOURCE_DIR}/src/controller
        #        # ------------------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/src/monitor
        #        ${PROJECT_SOURCE_DIR}/src/monitor/network
        #        ${PROJECT_SOURCE_DIR}/src/monitor/network/collector
        #        ${PROJECT_SOURCE_DIR}/src/monitor/system
        #        ${PROJECT_SOURCE_DIR}/src/monitor/system/collector
        #        # ------------------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/src/network/client/tcp
        ${PROJECT_SOURCE_DIR}/src/network/client/udp
        ${PROJECT_SOURCE_DIR}/src/network/server/tcp
        ${PROJECT_SOURCE_DIR}/src/network/server/udp
        ${PROJECT_SOURCE_DIR}/src/network/client
        ${PROJECT_SOURCE_DIR}/src/network/server
        ${PROJECT_SOURCE_DIR}/src/network/control
        ${PROJECT_SOURCE_DIR}/src/network
        #        # ------------------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/src/parser/cli
        ${PROJECT_SOURCE_DIR}/src/parser/yaml
        #        # ------------------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/src/pattern
        ${PROJECT_SOURCE_DIR}/src/pattern/burst
        ${PROJECT_SOURCE_DIR}/src/pattern/constant
        ${PROJECT_SOURCE_DIR}/src/pattern/ramp
        ${PROJECT_SOURCE_DIR}/src/pattern/sinusoidal
        #        # ------------------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/src/session
        ${PROJECT_SOURCE_DIR}/src/session/client
        ${PROJECT_SOURCE_DIR}/src/session/server
        #        # ------------------------------------------------------------------
        #        ${PROJECT_SOURCE_DIR}/src/traffic
        #        # ------------------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/src/utils
        #        ${PROJECT_SOURCE_DIR}/src/utils/csv
        #        ${PROJECT_SOURCE_DIR}/src/utils/printer

)

# Link Boost libs
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Boost::system
        Boost::thread
        Threads::Threads
        yaml-cpp::yaml-cpp
        ${PCAP_LIB}
)

