/*
 * H9 project
 *
 * Copyright (C) 2024 Kamil Palkowski. All rights reserved.
 */

#pragma once

#include "config.h"

#include <memory>
#include <queue>
#include <string>

#include "bus_driver.h"
#include "busframe.h"
#include "metrics_collector.h"

class BusEndpoint {
  public:
    static constexpr int EMPTY_BUF = BusDriver::EMPTY_BUF;
    static constexpr int SOCKET_CLOSE = BusDriver::SOCKET_CLOSE;
    static constexpr int RECV_FRAME = BusDriver::RECV_FRAME;
    static constexpr int RECV_FRAME_DATA_IN_BUF = BusDriver::RECV_FRAME_DATA_IN_BUF;

    const std::string& name;
    const std::string& driver_name;

  private:
    BusDriver* driver;
    std::queue<std::shared_ptr<BusFrame>> pending_frames;

    MetricsCollector::counter_t& sent_frames_counter;
    MetricsCollector::counter_t& received_frames_counter;

  public:
    explicit BusEndpoint(BusDriver* driver);
    ~BusEndpoint();

    int open();
    int get_socket();
    void close();

    int send_frame(std::shared_ptr<BusFrame> busframe);
    int recv_frame(BusFrame** busframe);
};
