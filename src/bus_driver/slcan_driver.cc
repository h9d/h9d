/*
 * H9 project
 *
 * Created by SQ8KFH on 2019-04-28.
 *
 * Copyright (C) 2019-2023 Kamil Palkowski. All rights reserved.
 */

#include "slcan_driver.h"

#include <fcntl.h>
#include <iomanip>
#include <spdlog/spdlog.h>
#include <sstream>
#include <system_error>
#include <termios.h>
#include <unistd.h>

SlcanDriver::SlcanDriver(const std::string& name, const std::string& tty, const std::string& init_string):
    BusDriver(name, "SLCAN"),
    _init_string(init_string),
    _tty(tty),
    pending_send_count(0) {
    noblock = false;
}

int SlcanDriver::open() {
    socket_fd = ::open(_tty.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (socket_fd == -1) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }
    else {
        if (noblock)
            fcntl(socket_fd, F_SETFL, O_NONBLOCK);
        else
            fcntl(socket_fd, F_SETFL, 0);
    }

    termios options;

    tcgetattr(socket_fd, &options);

    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    options.c_cflag |= (CLOCAL | CREAD); /* Enable the receiver and set local mode */
    /*
     * Select 8N1
     */
    options.c_iflag &= ~IGNBRK; // disable break processing

    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    options.c_cflag &= ~CRTSCTS;                /* Disable hardware flow control */
    options.c_iflag &= ~(IXON | IXOFF | IXANY); /* Disable software flow control */

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /* Raw Input */

    options.c_iflag = 0;
    options.c_lflag = 0;
    options.c_oflag = 0; /* Raw Output */

    options.c_cc[VMIN] = noblock ? 0 : 1;
    options.c_cc[VTIME] = 1;

    if (tcsetattr(socket_fd, TCSANOW, &options) != 0) {
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    if (_init_string != "") {
        ssize_t nbyte = write(socket_fd, _init_string.c_str(), _init_string.size());
        if (nbyte <= 0) {
            if (nbyte == 0 || errno == ENXIO) {
                close();
            }
            throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
        }
        SPDLOG_LOGGER_INFO(logger, "Using init command: {:?}", _init_string.c_str());
    }

    return socket_fd;
}

std::string SlcanDriver::build_slcan_msg(const H9Frame& frame) {
    uint32_t id = frame.can_id();

    std::ostringstream buf;
    buf << 'T';
    buf << std::setfill('0') << std::hex;
    buf << std::setw(8) << id;
    buf << std::setw(1) << static_cast<std::uint32_t>(frame.dlc());
    for (int i = 0; i < frame.dlc(); ++i) {
        buf << std::setw(2) << static_cast<std::uint32_t>(frame.data()[i]);
    }
    buf << "\r";
    return buf.str();
}

bool SlcanDriver::parse_slcan_msg(const std::string& slcan_data, H9Frame* frame) {
    if (slcan_data.size() < 10)
        return false;

    uint32_t id = std::stoi(slcan_data.substr(1, 8), nullptr, 16);
    uint8_t dlc = std::stoi(slcan_data.substr(9, 1), nullptr, 16);

    if (slcan_data.size() < (10 + 2*dlc))
        return false;

    uint8_t buf[H9Frame::MAX_DATA_LENGTH];

    for (int i = 0; i < dlc; ++i) {
        uint32_t tmp = std::stoi(slcan_data.substr(10 + i * 2, 2), nullptr, 16);
        buf[i] = static_cast<std::uint8_t>(tmp);
    }

    frame->can_id(id);
    frame->dlc(dlc);
    frame->data(buf);

    return true;
}

int SlcanDriver::recv_data(H9Frame& frame) {
    if (recv_queue.empty()) {
        std::uint8_t buf[100];
        ssize_t nbyte = read(socket_fd, buf, sizeof(buf) - 1);
        if (nbyte <= 0) {
            if (nbyte == 0 || errno == ENXIO) {
                close();
            }
            throw std::system_error(errno, std::system_category(), std::to_string(errno) + " " + __FILE__ + std::string(":") + std::to_string(__LINE__));
        }
        buf[nbyte] = '\0';
//        SPDLOG_TRACE("recv raw({}): {}", nbyte, (char*)&buf[0]);
        for (int i = 0; i < nbyte; ++i) {
            recv_buf.push_back(buf[i]);
            if (buf[i] == '\r' || buf[i] == '\a') {
                parse_buf();
                recv_buf.clear();
            }
        }
    }
    if (!recv_queue.empty()) {
        frame = recv_queue.front();
        recv_queue.pop();

        return recv_queue.empty() ? RECV_FRAME : RECV_FRAME_DATA_IN_BUF;
    }
    return EMPTY_BUF;
}

int SlcanDriver::send_data(H9Frame& frame) {
    std::string buf = build_slcan_msg(frame);
    ssize_t nbyte = write(socket_fd, buf.c_str(), buf.size());
    // std::cout << "send raw: " << buf.c_str() << std::endl;
    if (nbyte <= 0) {
        if (nbyte == 0 || errno == ENXIO) {
            close();
        }
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    ++pending_send_count;

    return nbyte;
}

void SlcanDriver::parse_buf() {
    if (recv_buf[0] == '\r') {
        if (pending_send_count > 0) {
            --pending_send_count;
            frame_sent_correctly();
        }
        //SPDLOG_LOGGER_ERROR(logger, "[CR]");
    }
    else if (recv_buf[0] == '\a') {
        if (pending_send_count > 0) {
            --pending_send_count;
            frame_sent_incorrectly();
        }
        //SPDLOG_LOGGER_ERROR(logger, "[BELL]");
    }
    else if (recv_buf[0] == 'T') {
        H9Frame frame;
        if (parse_slcan_msg(recv_buf, &frame)) {
            frame.origin(name);
            recv_queue.push(frame);
        }
        else {
            SPDLOG_LOGGER_ERROR(logger, "Recv malformed SLCAN command: '{}' from {}.", recv_buf.replace(recv_buf.find('\r'), 1, "\\r"), name);
        }
    }
}

void SlcanDriver::send_ack() {
    ssize_t nbyte = write(socket_fd, "\r", 1);
    if (nbyte <= 0) {
        if (nbyte == 0 || errno == ENXIO) {
            close();
        }
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }
}
