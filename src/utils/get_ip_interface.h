#pragma once

#include <string>

#ifdef __linux__

#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

inline std::string get_ip_of_interface(const std::string &interface_name) {
    struct ifaddrs *ifaddr, *ifa;
    std::string ip;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return "";
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;

        if (ifa->ifa_addr->sa_family == AF_INET &&
            interface_name == ifa->ifa_name) {
            char addr[INET_ADDRSTRLEN];
            void *sin_addr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, sin_addr, addr, INET_ADDRSTRLEN);
            ip = addr;
            break;
            }
    }

    freeifaddrs(ifaddr);
    return ip;
}

#else

inline std::string get_ip_of_interface(const std::string &) {
    // Interface IP selection not supported on non-Linux systems
    return "";
}

#endif
