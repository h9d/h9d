/*
 * H9 project
 *
 * Copyright (C) 2024 Kamil Palkowski. All rights reserved.
 */

#pragma once

#include "bus_driver.h"
#include "json_tcp_socket.h"

/**
 * @brief H9D driver – connection to another h9d daemon instance via JSONTCPSocket.
 *
 * Enables cascading h9d instances: one daemon can act as a client of another,
 * forwarding and receiving frames through the H9 message protocol.
 * Uses message IDs (`next_msg_id`) to correlate requests with responses.
 */
class H9DDriver: public BusDriver {
  private:
    JSONTCPSocket h9socket;
    int next_msg_id;
    std::string entity;
  public:
    H9DDriver(const std::string& name, std::string hostname, std::string port, const std::string& entity);
    int open() override;
    void close() override;

  private:
    int recv_data(H9Frame& frame) override;
    int send_data(H9Frame& frame) override;
};
