/*
 * H9 project
 *
 * Created by SQ8KFH on 2020-11-20.
 *
 * Copyright (C) 2020-2023 Kamil Palkowski. All rights reserved.
 */

#include "json_tcp_socket.h"

#include <jsonrpcpp/jsonrpcpp.hpp>
#include <sys/errno.h>
#include <sys/socket.h>
#include <system_error>

JSONTCPSocket::JSONTCPSocket(int socket):
    TCPSocket(socket) {
    if (connect() < 0)
        throw std::system_error(errno, std::generic_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
}

JSONTCPSocket::JSONTCPSocket(std::string hostname, std::string port) noexcept:
    TCPSocket(std::move(hostname), std::move(port)) {
}

int JSONTCPSocket::get_socket() noexcept {
    return _socket;
}

int JSONTCPSocket::authentication(const std::string& entity) {
    jsonrpcpp::Id id(1);

    jsonrpcpp::Request r(id, "authenticate", nlohmann::json({{"entity", entity}}));
    if (send(r.to_json()) <= 0) {
        //SPDLOG_ERROR("Authentication failed: {}.", std::strerror(errno));
        throw std::system_error(errno, std::system_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    nlohmann::json json;
    int res = recv_complete_msg(json);
    if (res <= 0) {
        //SPDLOG_ERROR("Authentication failed: {}.", std::strerror(errno));
        throw std::system_error(errno, std::system_category(), __FILE__ + std::string(":") + std::to_string(__LINE__));
    }

    if (json.is_discarded()) {
        //SPDLOG_ERROR("Authentication failed: invalid JSON.");
        throw std::runtime_error("Invalid JSON");
    }

    jsonrpcpp::Parser parser;
    jsonrpcpp::entity_ptr raw_res = parser.parse_json(json);

    if (raw_res->is_response()) {
        jsonrpcpp::response_ptr msg = std::dynamic_pointer_cast<jsonrpcpp::Response>(raw_res);
        auto result = msg->result();
        if (result.count("authentication")) {
            return result["authentication"].get<bool>();
        }
    }

    return 0;
}

int JSONTCPSocket::send(const nlohmann::json& json) noexcept {
    return TCPSocket::send(json.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));
}

int JSONTCPSocket::recv(nlohmann::json& json, int timeout_in_seconds) noexcept {
    std::string raw_str;
    int res = TCPSocket::recv(raw_str, timeout_in_seconds);
    if (res > 0) {
        json = std::move(nlohmann::json::parse(std::move(raw_str), nullptr, false));
    }
    return res;
}

int JSONTCPSocket::recv_complete_msg(nlohmann::json& json) noexcept {
    while (true) {
        int res = recv(json, 0);
        if (res < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            continue;
        }
        else {
            return res;
        }
    }
}

void JSONTCPSocket::shutdown_read() noexcept {
    shutdown(_socket, SHUT_RD);
}
