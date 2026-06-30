/*
 * H9 project
 *
 * Created by SQ8KFH on 2020-11-08.
 *
 * Copyright (C) 2020-2024 Kamil Palkowski. All rights reserved.
 */

#include "raw_node.h"

#include <arpa/inet.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <h9def.h>

#include "bus.h"
#include "node_mgr.h"

void RawNode::on_frame_recv(const H9Frame& frame) {
    frame_promise_set_mtx.lock();
    for (auto it = frame_promise_set.begin(); it != frame_promise_set.end();) {
        if ((*it)->on_frame(frame)) {
            it = frame_promise_set.erase(it);
        }
        else {
            ++it;
        }
    }
    frame_promise_set_mtx.unlock();
}

RawNode::FramePromise* RawNode::create_frame_promise(H9FrameComparator comparator) {
    auto p = new FramePromise(this, comparator);
    frame_promise_set_mtx.lock();
    frame_promise_set.insert(p);
    frame_promise_set_mtx.unlock();

    return p;
}

void RawNode::destroy_frame_promise(FramePromise* frame_promise) {
    frame_promise_set_mtx.lock();
    frame_promise_set.erase(frame_promise);
    delete frame_promise;
    frame_promise_set_mtx.unlock();
}

RawNode::RawNode(NodeMgr* node_mgr, Bus* bus, std::uint8_t node_id) noexcept:
    node_mgr(node_mgr),
    bus(bus),
    _node_id(node_id) {
}

std::uint8_t RawNode::node_id() const noexcept {
    return _node_id;
}

ssize_t RawNode::reset(const std::string& origin) {
    H9FrameComparator comparator;
    comparator.set_source_id(_node_id);
    comparator.set_type(H9Frame::Type::COMMAND_ERROR);
    comparator.set_type_in_alternate_set(H9Frame::Type::NODE_TURNED_ON);
    comparator.seqnum_override_for_alternate_set(0);

    FramePromise* frame_promise = create_frame_promise(comparator);

    H9Frame req(origin, H9Frame::Type::NODE_RESET, H9Frame::Flags::SINGE_FRAME, _node_id);

    int seqnum = bus->send_frame(req);
    frame_promise->set_comparator_seqnum(seqnum);

    auto future = frame_promise->get_future();

    if (future.wait_for(std::chrono::seconds(node_mgr->response_timeout_duration())) != std::future_status::ready) {
        destroy_frame_promise(frame_promise);
        return TIMEOUT_ERROR; // timeout
    }

    H9Frame res = future.get();

    destroy_frame_promise(frame_promise);

    if (res.type() == H9Frame::Type::NODE_TURNED_ON) {
        // TODO: zrobic cos z data?
        return res.dlc();
    }
    else if (res.type() == H9Frame::Type::COMMAND_ERROR && res.dlc() == 1) {
        return -res.data()[0];
    }

    return MALFORMED_FRAME_ERROR;
}

// ssize_t RawNode::discovery(const std::string& origin, std::uint16_t& type, std::uint16_t& version_major, std::uint16_t& version_minor, char& hardware_revision) {
//     H9FrameComparator comparator;
//     comparator.set_source_id(_node_id);
//     comparator.set_type(H9Frame::Type::NODE_INFO);
//     comparator.set_type_in_alternate_set(H9Frame::Type::COMMAND_ERROR);
//
//     FramePromise* frame_promise = create_frame_promise(comparator);
//
//     H9Frame req(origin, H9Frame::Type::DISCOVER, _node_id, {});
//
//     int seqnum = bus->send_frame(req);
//     frame_promise->set_comparator_seqnum(seqnum);
//
//     auto future = frame_promise->get_future();
//
//     if (future.wait_for(std::chrono::seconds(node_mgr->response_timeout_duration())) != std::future_status::ready) {
//         destroy_frame_promise(frame_promise);
//         return TIMEOUT_ERROR; // timeout
//     }
//
//     H9Frame res = future.get();
//
//     destroy_frame_promise(frame_promise);
//
//     if (res.type() == H9Frame::Type::NODE_INFO && res.dlc() > 6) {
//         uint8_t rr;
//         parse_node_info_frame(res, type, version_major, version_minor, hardware_revision, rr);
//         return res.dlc();
//     }
//     else if (res.type() == H9Frame::Type::COMMAND_ERROR && res.dlc() == 1) {
//         return -res.data()[0];
//     }
//
//     return MALFORMED_FRAME_ERROR;
// }

// int32_t RawNode::get_node_type(const std::string& origin) noexcept {
//     std::uint16_t buf;
//     ssize_t ret = get_reg(origin, NODE_TYPE_STD_REGISTER, sizeof(buf), reinterpret_cast<std::uint8_t*>(&buf));
//     if (ret == 2) {
//         return ntohs(buf);
//     }
//     else if (ret >= 0) {
//         return MALFORMED_FRAME_ERROR;
//     }
//     return ret;
// }
//
// int64_t RawNode::get_node_version(const std::string& origin, std::uint16_t* major, std::uint16_t* minor, std::uint16_t* patch) noexcept {
//     std::uint16_t buf[3];
//     ssize_t ret = get_reg(origin, NODE_VERSION_STD_REGISTER, sizeof(buf), reinterpret_cast<std::uint8_t*>(&buf));
//     if (ret == 6) {
//         std::uint16_t tmp = ntohs(buf[0]);
//         ret = tmp;
//         if (major)
//             *major = tmp;
//
//         tmp = ntohs(buf[1]);
//         ret = (ret << 16) | tmp;
//         if (minor)
//             *minor = tmp;
//
//         tmp = ntohs(buf[2]);
//         ret = (ret << 16) | tmp;
//         if (patch)
//             *patch = tmp;
//     }
//     else if (ret >= 0) {
//         return MALFORMED_FRAME_ERROR;
//     }
//     return ret;
// }
//
// int32_t RawNode::get_mcu_type(const std::string& origin) noexcept {
//     std::uint16_t buf;
//     ssize_t ret = get_reg(origin, NODE_MCU_TYPE_STD_REGISTER, sizeof(buf), reinterpret_cast<std::uint8_t*>(&buf));
//     if (ret == 2) {
//         return ntohs(buf);
//     }
//     else if (ret >= 0) {
//         return MALFORMED_FRAME_ERROR;
//     }
//     return ret;
// }

void RawNode::firmware_update(const std::string& origin, void (*progress_callback)(int percentage)) {
    // TODO: implement frimware update
}

ssize_t RawNode::bit_operation(const std::string& origin, H9Frame::Type type, std::uint8_t reg, std::uint8_t bit, std::size_t length, std::uint8_t* reg_after_set) {
    H9FrameComparator comparator;
    comparator.set_source_id(_node_id);
    comparator.set_type(H9Frame::Type::REG_VALUE);
    comparator.set_first_data_byte(reg);
    comparator.set_type_in_alternate_set(H9Frame::Type::COMMAND_ERROR);

    FramePromise* frame_promise = create_frame_promise(comparator);

    H9Frame req(origin, type, H9Frame::Flags::SINGE_FRAME, _node_id, {reg, bit});

    int seqnum = bus->send_frame(req);
    frame_promise->set_comparator_seqnum(seqnum);

    auto future = frame_promise->get_future();

    if (future.wait_for(std::chrono::seconds(node_mgr->response_timeout_duration())) != std::future_status::ready) {
        destroy_frame_promise(frame_promise);
        return TIMEOUT_ERROR; // timeout
    }

    H9Frame res = future.get();

    destroy_frame_promise(frame_promise);

    if (res.type() == H9Frame::Type::REG_VALUE && res.dlc() > 1) {
        if (reg_after_set) {
            size_t max = length < res.dlc() - 1 ? length : res.dlc() - 1;

            for (int i = 0; i < max; ++i) {
                reg_after_set[i] = res.data()[i + 1];
            }
        }

        return res.dlc() - 1;
    }
    else if (res.type() == H9Frame::Type::COMMAND_ERROR && res.dlc() == 1) {
        return -res.data()[0];
    }

    return MALFORMED_FRAME_ERROR;
}

ssize_t RawNode::set_bit(const std::string& origin, std::uint8_t reg, std::uint8_t bit, std::size_t length, std::uint8_t* reg_after_set) {
    return bit_operation(origin, H9Frame::Type::SET_BIT, reg, bit, length, reg_after_set);
}

ssize_t RawNode::clear_bit(const std::string& origin, std::uint8_t reg, std::uint8_t bit, std::size_t length, std::uint8_t* reg_after_set) {
    return bit_operation(origin, H9Frame::Type::CLEAR_BIT, reg, bit, length, reg_after_set);
}

ssize_t RawNode::set_reg(const std::string& origin, std::uint8_t reg, std::size_t length, const std::uint8_t* reg_val, std::uint8_t* reg_after_set, ssize_t reg_after_set_length) {
    H9FrameComparator comparator;
    comparator.set_source_id(_node_id);
    comparator.set_type(H9Frame::Type::REG_VALUE);
    comparator.set_first_data_byte(reg);
    comparator.set_type_in_alternate_set(H9Frame::Type::COMMAND_ERROR);

    FramePromise* frame_promise = create_frame_promise(comparator);

    std::vector<std::uint8_t> data = {reg};
    data.insert(data.end(), reg_val, &reg_val[length]);

    H9Frame req(origin, H9Frame::Type::SET_REG, H9Frame::Flags::SINGE_FRAME, _node_id, data);

    int seqnum = bus->send_frame(req);
    frame_promise->set_comparator_seqnum(seqnum);

    auto future = frame_promise->get_future();

    if (future.wait_for(std::chrono::seconds(node_mgr->response_timeout_duration())) != std::future_status::ready) {
        destroy_frame_promise(frame_promise);
        return TIMEOUT_ERROR; // timeout
    }

    H9Frame res = future.get();

    destroy_frame_promise(frame_promise);

    if (res.type() == H9Frame::Type::REG_VALUE && res.dlc() > 1) {
        if (reg_after_set) {
            reg_after_set_length = reg_after_set_length < 0 ? length : reg_after_set_length;
            size_t max = length < res.dlc() - 1 ? reg_after_set_length : res.dlc() - 1;

            for (int i = 0; i < max; ++i) {
                reg_after_set[i] = res.data()[i + 1];
            }
        }

        return res.dlc() - 1;
    }
    else if (res.type() == H9Frame::Type::COMMAND_ERROR && res.dlc() == 1) {
        return -res.data()[0];
    }

    return MALFORMED_FRAME_ERROR;
}

ssize_t RawNode::set_reg(const std::string& origin, std::uint8_t reg, std::uint8_t reg_val, std::uint8_t* reg_after_set) {
    return set_reg(origin, reg, sizeof(reg_val), &reg_val, reg_after_set);
}

ssize_t RawNode::set_reg(const std::string& origin, std::uint8_t reg, std::uint16_t reg_val, std::uint16_t* reg_after_set) {
    std::uint16_t buf = htons(reg_val);
    ssize_t ret = set_reg(origin, reg, sizeof(buf), reinterpret_cast<std::uint8_t*>(&buf), reinterpret_cast<std::uint8_t*>(reg_after_set));
    if (reg_after_set) {
        *reg_after_set = ntohs(*reg_after_set);
    }
    return ret;
}

ssize_t RawNode::set_reg(const std::string& origin, std::uint8_t reg, std::uint32_t reg_val, std::uint32_t* reg_after_set) {
    std::uint32_t buf = htonl(reg_val);
    ssize_t ret = set_reg(origin, reg, sizeof(buf), reinterpret_cast<std::uint8_t*>(&buf), reinterpret_cast<std::uint8_t*>(reg_after_set));
    if (reg_after_set) {
        *reg_after_set = ntohl(*reg_after_set);
    }
    return ret;
}

ssize_t RawNode::set_reg(const std::string& origin, std::uint8_t reg, float reg_val, float* reg_after_set) {
    uint32_t tmp_in, tmp_out;
    memcpy(&tmp_in, &reg_val, 4);

    ssize_t ret = set_reg(origin, reg, tmp_in, &tmp_out);

    if (reg_after_set) {
        memcpy(reg_after_set, &tmp_out, 4);
    }
    return ret;
}

ssize_t RawNode::get_reg(const std::string& origin, std::uint8_t reg, std::size_t length, std::uint8_t* reg_val) {
    H9FrameComparator comparator;
    comparator.set_source_id(_node_id);
    comparator.set_type(H9Frame::Type::REG_VALUE);
    comparator.set_first_data_byte(reg);
    comparator.set_type_in_alternate_set(H9Frame::Type::COMMAND_ERROR);

    FramePromise* frame_promise = create_frame_promise(comparator);

    H9Frame req(origin, H9Frame::Type::GET_REG, H9Frame::Flags::SINGE_FRAME, _node_id, {reg});

    int seqnum = bus->send_frame(req);
    frame_promise->set_comparator_seqnum(seqnum);

    auto future = frame_promise->get_future();

    if (future.wait_for(std::chrono::seconds(node_mgr->response_timeout_duration())) != std::future_status::ready) {
        destroy_frame_promise(frame_promise);
        return TIMEOUT_ERROR; // timeout
    }

    H9Frame res = future.get();

    destroy_frame_promise(frame_promise);

    if (res.type() == H9Frame::Type::REG_VALUE && res.dlc() > 1) {
        size_t ret = length < res.dlc() - 1 ? length : res.dlc() - 1;

        for (int i = 0; i < ret; ++i) {
            reg_val[i] = res.data()[i + 1];
        }

        return res.dlc() - 1;
    }
    else if (res.type() == H9Frame::Type::COMMAND_ERROR && res.dlc() == 1) {
        return -res.data()[0];
    }

    return MALFORMED_FRAME_ERROR;
}

ssize_t RawNode::get_reg(const std::string& origin, std::uint8_t reg, std::uint8_t* reg_val) {
    return get_reg(origin, reg, 1, reg_val);
}

ssize_t RawNode::get_reg(const std::string& origin, std::uint8_t reg, std::uint16_t* reg_val) {
    std::uint16_t buf;
    ssize_t ret = get_reg(origin, reg, sizeof(buf), reinterpret_cast<std::uint8_t*>(&buf));
    *reg_val = ntohs(buf);
    return ret;
}

ssize_t RawNode::get_reg(const std::string& origin, std::uint8_t reg, std::uint32_t* reg_val) {
    std::uint32_t buf;
    ssize_t ret = get_reg(origin, reg, sizeof(buf), reinterpret_cast<std::uint8_t*>(&buf));
    *reg_val = ntohl(buf);
    return ret;
}

ssize_t RawNode::get_reg(const std::string& origin, std::uint8_t reg, float* reg_val) {
    uint32_t tmp_out;
    ssize_t ret = get_reg(origin, reg, &tmp_out);
    memcpy(reg_val, &tmp_out, 4);
    return ret;
}

int RawNode::parse_node_info_frame(const H9Frame& frame, std::uint16_t& node_type, std::uint16_t& version_major, std::uint16_t& version_minor, char& hardware_revision, std::uint8_t& reset_reason) {
//    if (frame.dlc() < 7)
//        return MALFORMED_FRAME_ERROR;
    node_type = frame.data()[0] << 8 | frame.data()[1];
    version_major = frame.data()[2] << 8 | frame.data()[3];
    version_minor = frame.data()[4] << 8 | frame.data()[5];

    hardware_revision = frame.data()[6];
    reset_reason = frame.data()[7];

    return 0;
}
