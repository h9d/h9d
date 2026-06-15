/*
 * H9 project
 *
 * Copyright (C) 2024 Kamil Palkowski. All rights reserved.
 */

#pragma once

#include "bus_driver.h"
#include "h9msgsocket.h"

/**
 * @brief H9D driver – connection to another h9d daemon instance via H9MsgSocket.
 *
 * Enables cascading h9d instances: one daemon can act as a client of another,
 * forwarding and receiving frames through the H9 message protocol.
 * Uses message IDs (`next_msg_id`) to correlate requests with responses.
 */
class H9DDriver: public BusDriver {
  private:
    H9MsgSocket h9socket;
    int next_msg_id;

  public:
    H9DDriver(const std::string& name, std::string hostname, std::string port);
    int open() override;
    void close() override;

  private:
    int recv_data(ExtH9Frame& frame) override;
    int send_data(ExtH9Frame& frame) override;
};
