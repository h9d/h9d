/*
 * H9 project
 *
 * Created by SQ8KFH on 2018-07-23.
 *
 * Copyright (C) 2018-2020 Kamil Palkowski. All rights reserved.
 */

#pragma once

#include "bus_driver.h"
#include <queue>

/**
 * @brief SLCAN driver – communication over a serial port (TTY) using the SLCAN protocol.
 *
 * SLCAN encodes CAN frames as ASCII text (e.g. `T1234567800AABB\r`).
 * The driver maintains an internal receive buffer and a queue of frames
 * decoded from successive SLCAN messages.
 * Send acknowledgements (`z`/`Z` characters from the interface) are handled
 * via the `send_ack_callback`.
 */
class SlcanDriver: public BusDriver {
  private:
    const std::string _tty;
    const std::string _init_string;
    bool noblock;
    std::string recv_buf;

    std::queue<h9frame_t> recv_queue;

    int pending_send_count;

  public:
    SlcanDriver(const std::string& name, const std::string& tty, const std::string& init_string);
    int open() override;

    static std::string build_slcan_msg(const h9frame_t& frame);
    static bool parse_slcan_msg(const std::string& slcan_data, h9frame_t* frame);

  private:
    int recv_data(ExtH9Frame& frame) override;
    int send_data(ExtH9Frame& frame) override;
    void parse_buf();
    void send_ack();
};
