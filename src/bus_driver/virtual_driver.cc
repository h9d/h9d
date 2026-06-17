/*
 * H9 project
 *
 * Created by crowx on 2023-09-23.
 *
 * Copyright (C) 2023 Kamil Palkowski. All rights reserved.
 */

#include "virtual_driver.h"

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>

VirtualDriver::VirtualDriver(const std::string& name, int socket, int second_end_socket):
    BusDriver(name, "virtual"),
    second_end_socket(second_end_socket) {
    socket_fd = socket;
}

int VirtualDriver::open() {
    //::close(second_end_socket); //kiedy zamkyem druga strone kqueue zwraca Bad file descriptor?
    return socket_fd;
}

int VirtualDriver::recv_data(ExtH9Frame& frame) {
    std::uint8_t buf[ExtH9Frame::SERIALIZATION_LENGTH];

    ssize_t ret = recv(socket_fd, buf, ExtH9Frame::SERIALIZATION_LENGTH, 0);
    if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }
    if (ret != 0)
        frame.deserialize(name, buf);
    return ret != 0 ? RECV_FRAME : SOCKET_CLOSE;
}

int VirtualDriver::send_data(ExtH9Frame& frame) {
    int ret = send(socket_fd, frame.serialize().data(), ExtH9Frame::SERIALIZATION_LENGTH, 0);
    if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }
    else {
        frame_sent_correctly();
    }

    return ret;
}