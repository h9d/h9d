/*
 * H9 project
 *
 * Copyright (C) 2026 Kamil Palkowski. All rights reserved.
 */

#pragma once

#include <string>
#include <sys/un.h>

#include "bus_driver.h"

/**
 * @brief Pipe driver – inter-process communication via a Unix domain socket (SOCK_DGRAM).
 *
 * Used to connect two h9d instances or other local H9 processes without going
 * through the network stack. Each side has its own local socket path (`local_path`)
 * and sends datagrams to the peer's path (`remote_path`).
 */
class PipeDriver: public BusDriver {
  private:
    const std::string local_path;
    const std::string remote_path;

    sockaddr_un remote_addr;

  public:
    explicit PipeDriver(const std::string& name, std::string local_path, std::string remote_path);
    ~PipeDriver();
    int open();

  private:
    int recv_data(ExtH9Frame& frame);
    int send_data(ExtH9Frame& frame);
};