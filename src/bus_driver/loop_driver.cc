/*
 * H9 project
 *
 * Created by SQ8KFH on 2019-04-28.
 *
 * Copyright (C) 2019-2023 Kamil Palkowski. All rights reserved.
 */

#include "loop_driver.h"

#include <cstdlib>
#include <cstring>
#include <system_error>

LoopDriver::LoopDriver(const std::string& name):
    BusDriver(name, "loop") {
    bzero(&loopback_addr, sizeof(loopback_addr));
    loopback_addr.sin_family = AF_INET;
    loopback_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    loopback_addr.sin_port = htons(0); // random port;
}

int LoopDriver::open() {
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (bind(socket_fd, (const struct sockaddr*)&loopback_addr, sizeof(loopback_addr)) == -1) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    socklen_t sa_len = sizeof(loopback_addr);
    if (getsockname(socket_fd, (struct sockaddr*)&loopback_addr, &sa_len) == -1) { // get drawn port number
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    };

    return socket_fd;
}

int LoopDriver::recv_data(ExtH9Frame& frame) {
    sockaddr_in tmp_addr;
    socklen_t len = sizeof(tmp_addr);
    bcopy(&loopback_addr, &tmp_addr, len);

    h9frame_t h9frame = {};
    int ret = recvfrom(socket_fd, &h9frame, sizeof(h9frame_t), 0, (struct sockaddr*)&tmp_addr, &len);
    if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    if (ret != 0)
        frame = ExtH9Frame(h9frame, "");

    return ret != 0 ? RECV_FRAME : SOCKET_CLOSE;
}

int LoopDriver::send_data(ExtH9Frame& frame) {
    int ret = sendto(socket_fd, &frame.frame(), sizeof(h9frame_t), 0, (const struct sockaddr*)&loopback_addr, sizeof(loopback_addr));

    if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }
    else {
        frame_sent_correctly();
    }

    return ret;
}
