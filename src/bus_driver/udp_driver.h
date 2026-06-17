/*
* H9 project
*
* Created by SQ8KFH on 2024-04-19.
*
* Copyright (C) 2024 Kamil Palkowski. All rights reserved.
*/

#pragma once

#include <netdb.h>

#include "bus_driver.h"

/**
 * @brief UDP driver – tunnelling H9 frames over an IP/UDP network.
 *
 * CAN frames are encapsulated in a `can_frame` structure (including the CAN ID header)
 * and transmitted as UDP datagrams. The CAN ID is serialised big-endian (htonl/ntohl).
 * Typical use: connecting to a physical CAN bus through a remote UDP↔CAN bridge.
 */
class UDPDriver: public BusDriver {
 private:
   const std::string local_port;
   const std::string remote_addr;
   const std::string remote_port;

   addrinfo *remote;

 public:
   explicit UDPDriver(const std::string& name, std::string local_port, std::string remote_addr, std::string remote_port);
   ~UDPDriver();
   int open();

 private:
   int recv_data(H9Frame& frame);
   int send_data(H9Frame& frame);
};
