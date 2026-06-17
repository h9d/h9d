/*
 * H9 project
 *
 * Created by crowx on 2023-09-23.
 *
 * Copyright (C) 2023 Kamil Palkowski. All rights reserved.
 */

#pragma once

#include "bus_driver.h"

/**
 * @brief Virtual driver – a pair of connected sockets within a single process.
 *
 * Created via `socketpair()` or a similar mechanism. One end (`socket`) is used
 * by VirtualDriver; the other end (`second_end_socket`) is used by the virtual
 * node (e.g. VirtualPyNode). This allows H9 nodes to be simulated directly
 * inside the daemon process without any physical CAN bus.
 */
class VirtualDriver: public BusDriver {
  private:
    int second_end_socket;
  public:
    explicit VirtualDriver(const std::string& name, int socket, int second_end_socket);
    int open();

  private:
    int recv_data(H9Frame& frame);
    int send_data(H9Frame& frame);
};
