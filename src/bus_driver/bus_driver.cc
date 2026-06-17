/*
 * H9 project
 *
 * Created by SQ8KFH on 2019-05-10.
 *
 * Copyright (C) 2019-2023 Kamil Palkowski. All rights reserved.
 */

#include "bus_driver.h"

#include <unistd.h>
#include <utility>

void BusDriver::frame_sent_correctly() {
    if (send_ack_callback)
        send_ack_callback(true);
}

void BusDriver::frame_sent_incorrectly() {
    if (send_ack_callback)
        send_ack_callback(false);
}

BusDriver::BusDriver(const std::string& name, std::string driver_name):
    name(name),
    next_seqnum(0),
    driver_name(std::move(driver_name)),
    socket_fd(-1) {

    logger = spdlog::get("bus");
    if (logger == nullptr) {
        logger = spdlog::default_logger();
    }
}

int BusDriver::get_socket() {
    return socket_fd;
}

void BusDriver::close() {
    if (socket_fd >= 0)
        ::close(socket_fd);
}

int BusDriver::send_frame(ExtH9Frame& frame) {
    if (frame.type() == ExtH9Frame::Type::SET_REG ||
        frame.type() == ExtH9Frame::Type::GET_REG ||
        frame.type() == ExtH9Frame::Type::SET_BIT ||
        frame.type() == ExtH9Frame::Type::CLEAR_BIT ||
        frame.type() == ExtH9Frame::Type::NODE_UPGRADE ||
        frame.type() == ExtH9Frame::Type::NODE_RESET) {

        //if (frame.invalid_member() & ExtH9Frame::VALID_SEQNUM) {
            frame.seqnum(next_seqnum);
            next_seqnum = (next_seqnum + 1) & ((1 << H9FRAME_SEQNUM_BIT_LENGTH) - 1);
        //}
    }
    return send_data(frame);
}

int BusDriver::recv_frame(ExtH9Frame& frame) {
    return recv_data(frame);
}
