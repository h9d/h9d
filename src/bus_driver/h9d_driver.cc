/*
 * H9 project
 *
 * Copyright (C) 2024 Kamil Palkowski. All rights reserved.
 */

#include "h9d_driver.h"

#include <nlohmann/json.hpp>
#include <system_error>
#include <utility>

H9DDriver::H9DDriver(const std::string& name, std::string hostname, std::string port):
    BusDriver(name, "H9D"),
    h9socket(std::move(hostname), std::move(port)),
    next_msg_id(1) {
}

int H9DDriver::open() {
    if (h9socket.connect() < 0) {
        throw std::system_error(errno, std::system_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }
    int res = h9socket.authentication("h9ddriver");
    if (res != 1) {
        throw std::runtime_error("Authentication fail");
    }

    socket_fd = h9socket.get_socket();

    nlohmann::json req = {
        {"jsonrpc", "2.0"},
        {"id", ++next_msg_id},
        {"method", "subscribe"},
        {"params", {{"event", "frame"}}}
    };
    if (h9socket.send(req) <= 0) {
        throw std::system_error(errno, std::system_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    return socket_fd;
}

void H9DDriver::close() {
    socket_fd = -1;
    h9socket.close();
}

int H9DDriver::recv_data(ExtH9Frame& frame) {
    nlohmann::json json;
    int res = h9socket.recv_complete_msg(json);
    if (res < 0) {
        throw std::system_error(errno, std::system_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }
    if (res == 0) {
        return SOCKET_CLOSE;
    }
    if (json.is_discarded()) {
        return EMPTY_BUF;
    }

    if (json.contains("method") && json["method"] == "on_frame") {
        frame = json["params"]["frame"].get<ExtH9Frame>();
        return RECV_FRAME;
    }
    return EMPTY_BUF;
}

int H9DDriver::send_data(ExtH9Frame& frame) {
    nlohmann::json req = {
        {"jsonrpc", "2.0"},
        {"id", ++next_msg_id},
        {"method", "send_frame"},
        {"params", {{"frame", frame}, {"raw", true}}}
    };

    int ret = h9socket.send(req);
    if (ret <= 0) {
        throw std::system_error(errno, std::system_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }
    frame_sent_correctly();
    return ret;
}
