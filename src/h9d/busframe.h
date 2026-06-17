/*
 * H9 project
 *
 * Created by SQ8KFH on 2020-05-08.
 *
 * Copyright (C) 2020-2023 Kamil Palkowski. All rights reserved.
 */

#pragma once

#include "config.h"

#include <future>
#include <spdlog/spdlog.h>
#include <sstream>

#include "ext_h9frame.h"
#include "send_frame_result.h"

class BusFrame: public ExtH9Frame {
  private:
    std::uint64_t orgin_client_id;
    std::uint64_t orgin_msg_id;

    std::promise<SendFrameResult> _send_promise;
    unsigned int _number_of_active_bus;
    unsigned int _send_counter;
    unsigned int _send_fail_counter;

    bool _local_frame;
    bool _raw;
    bool _activate_promise;
  public:
    BusFrame() = delete;
    BusFrame(BusFrame&& a) = default;
    // BusFrame(const BusFrame&) = delete;
    // BusFrame& operator=(const BusFrame&) = delete;
    //BusFrame(const h9frame_t& frame, const std::string& origin, std::uint64_t orgin_client_id, std::uint64_t orgin_msg_id);
    BusFrame(ExtH9Frame&& a, bool raw) noexcept;
    BusFrame& operator=(BusFrame&& a) = default;

    ~BusFrame();

    bool raw() const;
    bool local_origin_frame() const;
    void mark_as_local_origin();

    std::uint64_t get_orgin_client_id() const;
    std::uint64_t get_orgin_msg_id() const;

    std::promise<SendFrameResult>& get_send_promise();
    void activate_send_finish_promise(unsigned int active_buses);
    bool is_sent_finish() const;
    void inc_send_counter();
    void inc_send_fail_counter();
};

class SimpleJSONBusFrameWraper {
  public:
    const BusFrame* busframe;

    SimpleJSONBusFrameWraper(const BusFrame& frame):
        busframe(&frame) {}

    SimpleJSONBusFrameWraper(const BusFrame* frame):
        busframe(frame) {}
};

template<>
struct spdlog::fmt_lib::formatter<SimpleJSONBusFrameWraper>: spdlog::fmt_lib::formatter<std::string> {
    auto format(const SimpleJSONBusFrameWraper& frame, format_context& ctx) -> decltype(ctx.out()) {
        std::stringstream data_table;
        if (frame.busframe->dlc())
            data_table << static_cast<unsigned int>(frame.busframe->data()[0]);
        for (int i = 1; i < frame.busframe->dlc(); ++i)
            data_table << ", " << static_cast<unsigned int>(frame.busframe->data()[i]);

        if (frame.busframe->is_unicast()) {
            return spdlog::fmt_lib::format_to(ctx.out(),
                                              R"("origin": "{}", "frame": {{"type": {}, "type_name": "{}", "source_id": {}, "flags": {}, "destination_id": {}, "seqnum": {}, "dlc": {}, "data": [{}]}})",
                                              frame.busframe->origin(),
                                              ExtH9Frame::to_underlying(frame.busframe->type()),
                                              ExtH9Frame::type_to_string(frame.busframe->type()),
                                              frame.busframe->source_id(),
                                              ExtH9Frame::to_underlying(frame.busframe->flags()),
                                              frame.busframe->destination_id(),
                                              frame.busframe->seqnum(),
                                              frame.busframe->dlc(),
                                              data_table.str());
        }
        else {
            return spdlog::fmt_lib::format_to(ctx.out(),
                                              R"("origin": "{}", "frame": {{"type": {}, "type_name": "{}", "source_id": {}, "broadcast_group": {}, "dlc": {}, "data": [{}]}})",
                                              frame.busframe->origin(),
                                              ExtH9Frame::to_underlying(frame.busframe->type()),
                                              ExtH9Frame::type_to_string(frame.busframe->type()),
                                              frame.busframe->source_id(),
                                              frame.busframe->broadcast_group(),
                                              frame.busframe->dlc(),
                                              data_table.str());
        }
    }
};

template<>
struct spdlog::fmt_lib::formatter<BusFrame>: spdlog::fmt_lib::formatter<std::string> {
    auto format(const BusFrame& frame, format_context& ctx) -> decltype(ctx.out()) {
        std::stringstream data_table;
        if (frame.dlc())
            data_table << static_cast<unsigned int>(frame.data()[0]);
        for (int i = 1; i < frame.dlc(); ++i)
            data_table << " " << static_cast<unsigned int>(frame.data()[i]);

        if (frame.is_unicast()) {
            return spdlog::fmt_lib::format_to(ctx.out(),
                                              "origin: {} {}->{} seq: {} type: {} dlc: {} data: {}",
                                              frame.origin(),
                                              frame.source_id(),
                                              frame.destination_id(),
                                              frame.seqnum(),
                                              ExtH9Frame::type_to_string(frame.type()),
                                              frame.dlc(),
                                              data_table.str());
        }
        else {
            return spdlog::fmt_lib::format_to(ctx.out(),
                                              "origin: {} {}->[{}] type: {} dlc: {} data: {}",
                                              frame.origin(),
                                              frame.source_id(),
                                              frame.broadcast_group(),
                                              ExtH9Frame::type_to_string(frame.type()),
                                              frame.dlc(),
                                              data_table.str());
        }
    }
};
