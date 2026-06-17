/*
 * H9 project
 *
 * Created by SQ8KFH on 2018-07-23.
 *
 * Copyright (C) 2018-2023 Kamil Palkowski. All rights reserved.
 */

#pragma once

#include <netinet/in.h>

#include "bus_driver.h"

/**
 * @brief Loopback driver – internal UDP loop on localhost.
 *
 * Sends and receives frames over UDP on 127.0.0.1, enabling bus logic
 * to be tested without physical CAN hardware.
 * A frame sent by this driver is immediately visible as a received frame.
 */
class LoopDriver: public BusDriver {
  private:
    sockaddr_in loopback_addr;

  public:
    explicit LoopDriver(const std::string& name);
    int open();

  private:
    int recv_data(H9Frame& frame);
    int send_data(H9Frame& frame);
};
