/*
 * H9 project
 *
 * Copyright (C) 2024 Kamil Palkowski. All rights reserved.
 */

#include "h9d_driver.h"

#include <nlohmann/json.hpp>
#include <system_error>

H9DDriver::H9DDriver(const std::string& name, H9Connector& connector, const std::string& entity):
    BusDriver(name, "H9D"),
    _entity(entity),
    _connector(connector),
    next_msg_id(1) {
}

int H9DDriver::open() {
    if (!_connector.is_connected()) {
        _connector.connect(_entity);
    }

    socket_fd = _connector.get_socket_fd();

    jsonrpcpp::Request r(_connector.get_next_id(), "subscribe", {"event", "frame"});

    try {
        _connector.send(std::make_shared<jsonrpcpp::Request>(r));
    }
    catch (std::system_error& e) {
        SPDLOG_ERROR("Can not send request: {}.", e.code().message());
        exit(EXIT_FAILURE);
    }
    catch (std::runtime_error& e) {
        SPDLOG_ERROR("Can not send request: {}.", e.what());
        exit(EXIT_FAILURE);
    }

    return _connector.get_socket_fd();
}

void H9DDriver::close() {
    socket_fd = -1;
    _connector.close();
}

int H9DDriver::recv_data(H9Frame& frame) {
    jsonrpcpp::entity_ptr raw_msg;

    try {
        raw_msg = _connector.recv();
    }
    catch (std::system_error& e) {
        SPDLOG_ERROR("Error during message received: {}.", e.code().message());
        exit(EXIT_FAILURE);
    }
    catch (std::runtime_error& e) {
        SPDLOG_ERROR("Error during message received: {}.",  e.what());
        exit(EXIT_FAILURE);
    }

    if (raw_msg && raw_msg->is_notification()) {
        jsonrpcpp::notification_ptr notification = std::dynamic_pointer_cast<jsonrpcpp::Notification>(raw_msg);
        if (notification && notification->method() == "on_frame") {
            Json params = notification->params().to_json();
            if (params.contains("frame")) {
                frame.origin(name);
                params["frame"].get_to(frame);
                return RECV_FRAME;
            }
        }
    }

    return EMPTY_BUF;
}

int H9DDriver::send_data(H9Frame& frame) {
    socket_fd = _connector.get_socket_fd();

    jsonrpcpp::Request r(_connector.get_next_id(), "send_frame", nlohmann::json({{"frame", frame}, {"raw", true}}));

    try {
        _connector.send(std::make_shared<jsonrpcpp::Request>(r));
    }
    catch (std::system_error& e) {
        SPDLOG_ERROR("Can not send request: {}.", e.code().message());
        exit(EXIT_FAILURE);
    }
    catch (std::runtime_error& e) {
        SPDLOG_ERROR("Can not send request: {}.", e.what());
        exit(EXIT_FAILURE);
    }

    frame_sent_correctly();
    return 1;
    //return ret;
}
