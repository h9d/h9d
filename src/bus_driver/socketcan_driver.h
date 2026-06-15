/*
 * H9 project
 *
 * Created by SQ8KFH on 2019-04-09.
 *
 * Copyright (C) 2019-2023 Kamil Palkowski. All rights reserved.
 *
 * ========================================================
 *
 * ip link set can0 type can bitrate 125000
 * ip link set can0 txqueuelen 1000
 * ip link set up can0
 *
 * ip -details -statistics link show can0
 *
 */

#pragma once

#include "bus_driver.h"

/**
 * @brief SocketCAN driver – direct access to a CAN bus via Linux SocketCAN.
 *
 * Requires a loaded kernel module and a configured CAN network interface
 * (e.g. `can0`). Supports 29-bit extended frames (CAN_EFF_FLAG).
 *
 * Example interface setup:
 * @code
 * ip link set can0 type can bitrate 125000
 * ip link set can0 txqueuelen 1000
 * ip link set up can0
 * @endcode
 */
class SocketCANDriver: public BusDriver {
  private:
    const std::string _interface;

  public:
    explicit SocketCANDriver(const std::string& name, const std::string& interface);
    int open();

  private:
    int recv_data(ExtH9Frame& frame);
    int send_data(ExtH9Frame& frame);
};
