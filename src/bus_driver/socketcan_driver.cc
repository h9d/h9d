/*
 * H9 project
 *
 * Created by SQ8KFH on 2019-04-09.
 *
 * Copyright (C) 2019-2023 Kamil Palkowski. All rights reserved.
 */

#include "socketcan_driver.h"

#include <cstdlib>
#include <cstring>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

SocketCANDriver::SocketCANDriver(const std::string& name, const std::string& interface):
    BusDriver(name, "SocketCAN"),
    _interface(interface) {
}

int SocketCANDriver::open() {
    struct sockaddr_can addr;
    struct ifreq ifr;

    socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd == -1) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    strcpy(ifr.ifr_name, _interface.c_str());
    if (ioctl(socket_fd, SIOCGIFINDEX, &ifr) < 0) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__) + " '" + _interface + "'");
    }

    addr.can_family = PF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    struct can_filter rfilter[1];
    rfilter[0].can_id = CAN_EFF_FLAG; // only frame with extended id
    rfilter[0].can_mask = CAN_EFF_FLAG;

    if (setsockopt(socket_fd, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter)) < 0) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    return socket_fd;
}

int SocketCANDriver::recv_data(ExtH9Frame& frame) {
    struct can_frame can_msg;

    size_t nbyte = read(socket_fd, &can_msg, sizeof(can_frame));
    if (nbyte <= 0) {
        if (nbyte == 0 || errno == ENXIO) {
            close();
        }
        throw std::system_error(errno, std::system_category(), std::to_string(errno) + __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    h9frame_t h9frame = {};

    h9frame.type = (uint8_t)((can_msg.can_id >> (H9FRAME_ID_BIT_LENGTH + H9FRAME_FLAGS_BITS_LENGTH + H9FRAME_ID_BIT_LENGTH + H9FRAME_SEQNUM_BIT_LENGTH)) & ((1 << H9FRAME_TYPE_BIT_LENGTH) - 1));
    h9frame.source_id = static_cast<std::uint8_t>((can_msg.can_id >> (H9FRAME_FLAGS_BITS_LENGTH + H9FRAME_ID_BIT_LENGTH + H9FRAME_SEQNUM_BIT_LENGTH)) & ((1 << H9FRAME_ID_BIT_LENGTH) - 1));
    h9frame.unicast.flags = (uint8_t)((can_msg.can_id >> (H9FRAME_ID_BIT_LENGTH + H9FRAME_SEQNUM_BIT_LENGTH)) & ((1 << H9FRAME_FLAGS_BITS_LENGTH) - 1));
    h9frame.unicast.destination_id = static_cast<std::uint8_t>((can_msg.can_id >> H9FRAME_SEQNUM_BIT_LENGTH) & ((1 << H9FRAME_ID_BIT_LENGTH) - 1));
    h9frame.unicast.seqnum = static_cast<std::uint8_t>((can_msg.can_id >> 0) & ((1 << H9FRAME_SEQNUM_BIT_LENGTH) - 1));

    h9frame.dlc = can_msg.can_dlc;
    for (int i = 0; i < 8; i++) {
        h9frame.data[i] = can_msg.data[i];
    }

    frame = ExtH9Frame(h9frame, "");

    return nbyte != 0 ? RECV_FRAME : SOCKET_CLOSE;
}

int SocketCANDriver::send_data(ExtH9Frame& frame) {
    struct can_frame can_msg;
    memset(&can_msg, 0, sizeof(struct can_frame));

    can_msg.can_id |= ExtH9Frame::to_underlying(frame.type()) & ((1 << H9FRAME_TYPE_BIT_LENGTH) - 1);
    can_msg.can_id <<= H9FRAME_ID_BIT_LENGTH;
    can_msg.can_id |= frame.source_id() & ((1 << H9FRAME_ID_BIT_LENGTH) - 1);
    can_msg.can_id <<= H9FRAME_FLAGS_BITS_LENGTH;
    can_msg.can_id |= frame.flags() & ((1 << H9FRAME_FLAGS_BITS_LENGTH) - 1);
    can_msg.can_id <<= H9FRAME_ID_BIT_LENGTH;
    can_msg.can_id |= frame.destination_id() & ((1 << H9FRAME_ID_BIT_LENGTH) - 1);
    can_msg.can_id <<= H9FRAME_SEQNUM_BIT_LENGTH;
    can_msg.can_id |= frame.seqnum() & ((1 << H9FRAME_SEQNUM_BIT_LENGTH) - 1);

    can_msg.can_id |= CAN_EFF_FLAG;

    can_msg.can_dlc = frame.dlc();
    for (int i = 0; i < 8; i++) {
        can_msg.data[i] = frame.data()[i];
    }

    ssize_t nbyte = write(socket_fd, &can_msg, sizeof(can_frame));
    if (nbyte <= 0) {
        if (nbyte == 0 || errno == ENXIO) {
            close();
        }
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }
    frame_sent_correctly();

    return nbyte;
}
