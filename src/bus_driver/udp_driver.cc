/*
 * H9 project
 *
 * Created by SQ8KFH on 2024-04-19.
 *
 * Copyright (C) 2024 Kamil Palkowski. All rights reserved.
 */

#include "udp_driver.h"

#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <utility>

UDPDriver::UDPDriver(const std::string& name, std::string local_port, std::string remote_addr, std::string remote_port):
    BusDriver(name, "udp"),
    remote(nullptr),
    local_port(std::move(local_port)),
    remote_addr(std::move(remote_addr)),
    remote_port(std::move(remote_port)) {
}

UDPDriver::~UDPDriver() {
    if (remote) {
        freeaddrinfo(remote);
    }
}

int UDPDriver::open() {
    addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET6 to use IPv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    int ret;

    if ((ret = getaddrinfo(nullptr, local_port.c_str(), &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != nullptr; p = p->ai_next) {
        if ((socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(socket_fd, p->ai_addr, p->ai_addrlen) == -1) {
            ::close(socket_fd);
            perror("listener: bind");
            continue;
        }
        break;
    }

    if (p == nullptr) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    freeaddrinfo(servinfo);

    hints.ai_flags = 0;

    if ((ret = getaddrinfo(remote_addr.c_str(), remote_port.c_str(), &hints, &remote)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    return socket_fd;
}

int UDPDriver::recv_data(H9Frame& frame) {
    sockaddr_storage remote_addr;
    socklen_t len = sizeof(remote_addr);

    std::uint8_t buf[H9Frame::SERIALIZATION_LENGTH];

    ssize_t ret = recvfrom(socket_fd, buf, H9Frame::SERIALIZATION_LENGTH, 0, (struct sockaddr*)&remote_addr, &len);
    if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    if (ret != 0)
        frame.deserialize(name, buf);

    return ret != 0 ? RECV_FRAME : SOCKET_CLOSE;
}

int UDPDriver::send_data(H9Frame& frame) {
    ssize_t ret = sendto(socket_fd, frame.serialize().data(), H9Frame::SERIALIZATION_LENGTH, 0, remote->ai_addr, remote->ai_addrlen);

    if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }
    else {
        frame_sent_correctly();
    }

    return ret;
}
