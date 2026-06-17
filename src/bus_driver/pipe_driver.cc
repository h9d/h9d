/*
 * H9 project
 *
 * Copyright (C) 2026 Kamil Palkowski. All rights reserved.
 */

#include "pipe_driver.h"

#include <cstring>
#include <system_error>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

PipeDriver::PipeDriver(const std::string& name, std::string local_path, std::string remote_path):
    BusDriver(name, "pipe"),
    local_path(std::move(local_path)),
    remote_path(std::move(remote_path)) {
    memset(&remote_addr, 0, sizeof(remote_addr));
}

PipeDriver::~PipeDriver() {
    ::unlink(local_path.c_str());
}

int PipeDriver::open() {
    socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (socket_fd == -1) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    ::unlink(local_path.c_str());

    sockaddr_un local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sun_family = AF_UNIX;
    strncpy(local_addr.sun_path, local_path.c_str(), sizeof(local_addr.sun_path) - 1);

    if (bind(socket_fd, reinterpret_cast<const sockaddr*>(&local_addr), sizeof(local_addr)) == -1) {
        ::close(socket_fd);
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    remote_addr.sun_family = AF_UNIX;
    strncpy(remote_addr.sun_path, remote_path.c_str(), sizeof(remote_addr.sun_path) - 1);

    return socket_fd;
}

int PipeDriver::recv_data(H9Frame& frame) {
    std::uint8_t buf[H9Frame::SERIALIZATION_LENGTH];

    ssize_t ret = recv(socket_fd, buf, H9Frame::SERIALIZATION_LENGTH, 0);
    if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }
    if (ret != 0)
        frame.deserialize(name, buf);
    return ret != 0 ? RECV_FRAME : SOCKET_CLOSE;
}

int PipeDriver::send_data(H9Frame& frame) {
    ssize_t ret = sendto(socket_fd, frame.serialize().data(), H9Frame::SERIALIZATION_LENGTH, 0,
                         reinterpret_cast<const sockaddr*>(&remote_addr), sizeof(remote_addr));
    if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }
    else {
        frame_sent_correctly();
    }
    return static_cast<int>(ret);
}