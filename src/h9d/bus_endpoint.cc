/*
 * H9 project
 *
 * Copyright (C) 2024 Kamil Palkowski. All rights reserved.
 */

#include "bus_endpoint.h"

BusEndpoint::BusEndpoint(BusDriver* driver):
    driver(driver),
    name(driver->name),
    driver_name(driver->driver_name),
    sent_frames_counter(MetricsCollector::make_counter("bus.endpoints[name=" + driver->name + "].send_frames")),
    received_frames_counter(MetricsCollector::make_counter("bus.endpoints[name=" + driver->name + "].received_frames")) {

    driver->send_ack_callback = [this](bool success) {
        if (!pending_frames.empty()) {
            auto frame = pending_frames.front();
            pending_frames.pop();
            if (success)
                frame->inc_send_counter();
            else
                frame->inc_send_fail_counter();
        }
    };
}

BusEndpoint::~BusEndpoint() {
    delete driver;
}

int BusEndpoint::open() {
    return driver->open();
}

int BusEndpoint::get_socket() {
    return driver->get_socket();
}

void BusEndpoint::close() {
    driver->close();
}

int BusEndpoint::send_frame(std::shared_ptr<BusFrame> busframe) {
    ++sent_frames_counter;
    pending_frames.push(busframe);
    return driver->send_frame(*busframe);
}

int BusEndpoint::recv_frame(BusFrame** busframe) {
    assert(*busframe == nullptr);

    ExtH9Frame frame;
    int ret = driver->recv_frame(frame);
    if (ret >= BusDriver::RECV_FRAME) {
        *busframe = new BusFrame(std::move(frame), false);
        ++received_frames_counter;
    }
    return ret;
}
