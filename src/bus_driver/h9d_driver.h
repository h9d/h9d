/*
 * H9 project
 *
 * Copyright (C) 2024 Kamil Palkowski. All rights reserved.
 */

#pragma once

#include "bus_driver.h"
#include "h9connector.h"

class H9DDriver: public BusDriver {
  private:
    H9Connector& _connector;
    std::string _entity;
    int next_msg_id;
  public:
    H9DDriver(const std::string& name, H9Connector& connector, const std::string& entity);
    int open() override;
    void close() override;

  private:
    int recv_data(H9Frame& frame) override;
    int send_data(H9Frame& frame) override;
};
