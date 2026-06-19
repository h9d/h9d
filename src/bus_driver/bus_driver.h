/*
 * H9 project
 *
 * Created by SQ8KFH on 2019-04-28.
 *
 * Copyright (C) 2019-2023 Kamil Palkowski. All rights reserved.
 */

#pragma once

#include "config.h"

#include <functional>
#include <spdlog/spdlog.h>
#include <string>

#include "h9_frame.h"

/**
 * @brief Abstract base class representing an H9 CAN bus driver.
 *
 * BusDriver defines the interface for sending and receiving H9 frames over
 * various physical or virtual transport channels. Each concrete driver
 * (SLCAN, SocketCAN, UDP, …) inherits from this class and implements
 * `recv_data()` and `send_data()`.
 *
 * Sequence numbers (seqnum) are assigned automatically by `send_frame()`
 * for frame types that require acknowledgement (SET_REG, GET_REG, SET_BIT,
 * CLEAR_BIT, NODE_UPGRADE, NODE_RESET).
 *
 * Class hierarchy:
 * @code
 * BusDriver
 * ├── SlcanDriver      – SLCAN serial protocol (TTY)
 * ├── SocketCANDriver  – Linux SocketCAN
 * ├── UDPDriver        – UDP transport
 * ├── PipeDriver       – Unix domain socket (pair of local processes)
 * ├── LoopDriver       – UDP localhost loopback (testing)
 * ├── VirtualDriver    – in-process virtual driver
 * └── H9DDriver        – connection to another h9d instance
 * @endcode
 */
class BusDriver {
  public:
    /** Returned by recv_data/recv_frame when the internal buffer is empty. */
    static constexpr int EMPTY_BUF = -1;
    /** Returned when the connection has been closed by the remote side. */
    static constexpr int SOCKET_CLOSE = 0;
    /** Returned when a complete H9 frame has been received. */
    static constexpr int RECV_FRAME = 1;
    /** Returned when a frame was placed in the internal buffer; caller must invoke recv again. */
    static constexpr int RECV_FRAME_DATA_IN_BUF = 2;

    /**
     * @brief Callback invoked after each frame send attempt.
     *
     * The argument is `true` on success (`frame_sent_correctly()`)
     * or `false` on failure (`frame_sent_incorrectly()`).
     * If not set (nullptr), no callback is invoked.
     */
    std::function<void(bool)> send_ack_callback;

    // /** Next sequence number to assign (wraps modulo 2^H9FRAME_SEQNUM_BIT_LENGTH). */
    // std::uint8_t next_seqnum;

  protected:
    /** Socket/port file descriptor used by the driver; -1 when closed. */
    int socket_fd;

    std::shared_ptr<spdlog::logger> logger;

    /**
     * @brief Receive one frame from the transport channel.
     * @param[out] frame  The received H9 frame.
     * @return RECV_FRAME, RECV_FRAME_DATA_IN_BUF, or SOCKET_CLOSE.
     */
    virtual int recv_data(H9Frame& frame) = 0;

    /**
     * @brief Send a frame over the transport channel.
     * @param[in] frame  The frame to send.
     * @return Number of bytes sent, or a negative value on error.
     */
    virtual int send_data(H9Frame& frame) = 0;

    /** Invokes send_ack_callback(true) if the callback is set. */
    void frame_sent_correctly();
    /** Invokes send_ack_callback(false) if the callback is set. */
    void frame_sent_incorrectly();

  public:
    /** Driver type identifier (e.g. "slcan", "socketcan"). */
    const std::string driver_name;
    /** Instance name assigned in configuration (e.g. "can0"). */
    const std::string name;

    /**
     * @brief Constructor.
     * @param name         Instance name (from configuration).
     * @param driver_name  Driver type identifier.
     */
    explicit BusDriver(const std::string& name, std::string driver_name);
    virtual ~BusDriver() = default;

    /** Returns the socket file descriptor, suitable for use with select/epoll. */
    int get_socket();

    /**
     * @brief Open the connection to the bus.
     * @return 0 on success, negative value on error.
     */
    virtual int open() = 0;

    /** Close the socket file descriptor. */
    virtual void close();

    /**
     * @brief Send an H9 frame, assigning seqnum automatically when required.
     *
     * For frame types that require sequencing (SET_REG, GET_REG, SET_BIT,
     * CLEAR_BIT, NODE_UPGRADE, NODE_RESET), assigns `next_seqnum` if the
     * seqnum field in the frame is invalid (VALID_SEQNUM not set).
     *
     * @param[in,out] frame  Frame to send; the seqnum field may be filled in.
     * @return Result of send_data().
     */
    int send_frame(H9Frame& frame);

    /**
     * @brief Receive one H9 frame.
     * @param[out] frame  The received frame.
     * @return RECV_FRAME, RECV_FRAME_DATA_IN_BUF, or SOCKET_CLOSE.
     */
    int recv_frame(H9Frame& frame);
};
