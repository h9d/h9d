/*
 * H9 project
 *
 * Created by crowx on 2023-09-07.
 *
 * Copyright (C) 2023 Kamil Palkowski. All rights reserved.
 */

#include "ext_h9frame.h"
#include <h9def.h>

ExtH9Frame::ExtH9Frame():
    _frame({}),
    _creation_timestamp(timestamp_t::clock::now()),
    valid(0) {
}

ExtH9Frame::ExtH9Frame(const h9frame_t& frame, const std::string& origin):
    _frame(frame),
    _origin(origin),
    _creation_timestamp(timestamp_t::clock::now()) {
    valid = (VALID_TYPE | VALID_SEQNUM | VALID_DESTINATION_ID | VALID_DATA);
    if (origin != "")
        valid |= VALID_ORIGIN;
    if (_frame.source_id <= ExtH9Frame::H9FRAME_SOURCE_ID_MAX_VALUE)
        valid |= VALID_SOURCE_ID;
    if (_frame.dlc <= ExtH9Frame::H9FRAME_DATA_LENGTH)
        valid |= VALID_DLC;
}

ExtH9Frame::ExtH9Frame(const std::string& origin, ExtH9Frame::Type type, std::uint16_t dst, std::uint8_t dlc, const std::vector<std::uint8_t>& data):
    _frame({}),
    _creation_timestamp(timestamp_t::clock::now()) {
    valid = 0;
    this->origin(origin);
    this->type(type);
    this->destination_id(dst);
    this->dlc(dlc);
    this->data(data);
}

unsigned int ExtH9Frame::valid_member() {
    return (valid & (VALID_ORIGIN | VALID_TYPE | VALID_SEQNUM | VALID_DESTINATION_ID | VALID_SOURCE_ID | VALID_DLC | VALID_DATA)) | VALID_UNUSED;
}

unsigned int ExtH9Frame::invalid_member() {
    return ~valid & (VALID_ORIGIN | VALID_TYPE | VALID_SEQNUM | VALID_DESTINATION_ID | VALID_SOURCE_ID | VALID_DLC | VALID_DATA);
}

void ExtH9Frame::origin(const std::string& origin) {
    _origin = origin;
    if (_origin != "")
        valid |= VALID_ORIGIN;
}

void ExtH9Frame::type(ExtH9Frame::Type type) {
    _frame.type = ExtH9Frame::to_underlying(type);
    valid |= VALID_TYPE;
}

void ExtH9Frame::type(std::uint8_t type) {
    if (type <= ExtH9Frame::H9FRAME_TYPE_MAX_VALUE) {
        _frame.type = type;
        valid |= VALID_TYPE;
    }
}

void ExtH9Frame::seqnum(std::uint8_t seqnum) {
    if (seqnum <= ExtH9Frame::H9FRAME_SEQNUM_MAX_VALUE) {
        _frame.unicast.seqnum = seqnum;
        valid |= VALID_SEQNUM;
    }
    else {
        valid &= ~VALID_SEQNUM;
    }
}

void ExtH9Frame::destination_id(std::uint8_t destination_id) {
    //if (destination_id <= ExtH9Frame::H9FRAME_DESTINATION_ID_MAX_VALUE) {
        _frame.unicast.destination_id = destination_id;
        valid |= VALID_DESTINATION_ID;
    //}
}

void ExtH9Frame::broadcast_group(std::uint16_t broadcast_group) {
    _frame.broadcast.group = broadcast_group;
    valid |= VALID_BROADCAST_GROUP;
}

void ExtH9Frame::source_id(std::uint8_t source_id) {
    //if (source_id <= ExtH9Frame::H9FRAME_SOURCE_ID_MAX_VALUE) {
        _frame.source_id = source_id;
        valid |= VALID_SOURCE_ID;
    //}
}

void ExtH9Frame::dlc(std::uint8_t dlc) {
    if (dlc <= ExtH9Frame::H9FRAME_DATA_LENGTH) {
        _frame.dlc = dlc;
        valid |= VALID_DLC;
    }
}

void ExtH9Frame::data(const std::vector<std::uint8_t>& data) {
    int i = 0;
    for (auto& d : data) {
        _frame.data[i] = d;
        ++i;
        if (i > 8)
            break;
    }
    valid |= VALID_DATA;
}

bool ExtH9Frame::is_unicast() const {
    return !(_frame.type & H9FRAME_UNICAST_BROADCAST_BIT);
}

bool ExtH9Frame::is_broadcast() const {
    return _frame.type & H9FRAME_UNICAST_BROADCAST_BIT;
}

void to_json(nlohmann::json& j, const ExtH9Frame& f) {
    std::vector<std::uint8_t> data(f.data(), f.data() + f.dlc());
    j = nlohmann::json{{"origin", f.origin()},
                       {"type", ExtH9Frame::to_underlying(f.type())},
                       {"seqnum", f.seqnum()},
                       {"destination_id", f.destination_id()},
                       {"source_id", f.source_id()},
                       {"dlc", f.dlc()},
                       {"data", data}};
}

void from_json(const nlohmann::json& j, ExtH9Frame& f) {
    if (j.count("origin"))
        f.origin(j.at("origin").get<std::string>());
    else
        f.origin("");

    if (j.count("type"))
        f.type(j.at("type").get<std::uint8_t>());
    if (j.count("seqnum"))
        f.seqnum(j.at("seqnum").get<std::uint8_t>());
    if (j.count("destination_id"))
        f.destination_id(j.at("destination_id").get<std::uint16_t>());
    if (j.count("source_id"))
        f.source_id(j.at("source_id").get<std::uint16_t>());
    if (j.count("dlc"))
        f.dlc(j.at("dlc").get<std::uint8_t>());
    if (j.count("data"))
        f.data(j.at("data").get<std::vector<std::uint8_t>>());
}

const char* ExtH9Frame::type_to_string(ExtH9Frame::Type type) {
    switch (type) {
    case Type::RES1:
        return "RES1";
    case Type::PAGE_START:
        return "PAGE_START";
    case Type::QUIT_BOOTLOADER:
        return "QUIT_BOOTLOADER";
    case Type::PAGE_FILL:
        return "PAGE_FILL";
    case Type::BOOTLOADER_TURNED_ON:
        return "BOOTLOADER_TURNED_ON";
    case Type::PAGE_FILL_NEXT:
        return "PAGE_FILL_NEXT";
    case Type::PAGE_WRITED:
        return "PAGE_WRITED";
    case Type::PAGE_FILL_BREAK:
        return "PAGE_FILL_BREAK";
    case Type::COMMAND_ERROR:
        return "COMMAND_ERROR";
    case Type::REG_VALUE:
        return "REG_VALUE";
    case Type::SET_REG:
        return "SET_REG";
    case Type::GET_REG:
        return "GET_REG";
    case Type::SET_BIT:
        return "SET_BIT";
    case Type::CLEAR_BIT:
        return "CLEAR_BIT";
    case Type::NODE_UPGRADE:
        return "NODE_UPGRADE";
    case Type::NODE_RESET:
        return "NODE_RESET";
    case Type::DISCOVER:
        return "DISCOVER";
    case Type::GROUP_RESET:
        return "GROUP_RESET";
    case Type::NODE_FAULT:
        return "NODE_FAULT";
    case Type::REG_VALUE_BROADCAST:
        return "REG_VALUE_BROADCAST";
    case Type::NODE_HEARTBEAT:
        return "NODE_HEARTBEAT";
    case Type::NODE_INFO:
        return "NODE_INFO";
    case Type::NODE_TURNED_ON:
        return "NODE_TURNED_ON";
    case Type::RES2:
        return "RES2";
    case Type::NODE_SPECIFIC_BROADCAST0:
        return "NODE_SPECIFIC_BROADCAST0";
    case Type::NODE_SPECIFIC_BROADCAST1:
        return "NODE_SPECIFIC_BROADCAST1";
    case Type::NODE_SPECIFIC_BROADCAST2:
        return "NODE_SPECIFIC_BROADCAST2";
    case Type::NODE_SPECIFIC_BROADCAST3:
        return "NODE_SPECIFIC_BROADCAST3";
    case Type::NODE_SPECIFIC_BROADCAST4:
        return "NODE_SPECIFIC_BROADCAST4";
    case Type::NODE_SPECIFIC_BROADCAST5:
        return "NODE_SPECIFIC_BROADCAST5";
    case Type::NODE_SPECIFIC_BROADCAST6:
        return "NODE_SPECIFIC_BROADCAST6";
    case Type::NODE_SPECIFIC_BROADCAST7:
        return "NODE_SPECIFIC_BROADCAST7";
    }
    return nullptr;
}

const char* ExtH9Frame::error_to_string(ExtH9Frame::Error error) {
    switch (error) {
    case Error::INVALID_FRAME:
        return "INVALID FRAME";
    case Error::BOOTLOADER_UNSUPPORTED:
        return "BOOTLOADER UNSUPPORTED";
    case Error::UNSUPPORTED_OPERATION:
        return "UNSUPPORTED OPERATION";
    case Error::UNSUPPORTED_REGISTER:
        return "UNSUPPORTED REGISTER";
    case Error::INVALID_REGISTER:
        return "INVALID REGISTER";
    case Error::READ_ONLY_REGISTER:
        return "READ ONLY REGISTER";
    case Error::WRITE_ONLY_REGISTER:
        return "WRITE ONLY REGISTER";
    case Error::REGISTER_SIZE_MISMATCH:
        return "REGISTER SIZE MISMATCH";
    }
    return "UNKNOWN_ERROR";
}

const char* ExtH9Frame::mcu_type_to_string(std::uint8_t mcu) {
    switch (mcu) {
        case NODE_MCU_ATMEGA16M1:
            return "ATmega16M1";
        case NODE_MCU_ATMEGA32M1:
            return "ATmega32M1";
        case NODE_MCU_ATMEGA64M1:
            return "ATmega64M1";
        case NODE_MCU_ATMEGA16C1:
            return "ATmega16C1";
        case NODE_MCU_ATMEGA32C1:
            return "ATmega32C1";
        case NODE_MCU_ATMEGA64C1:
            return "ATmega64C1";
        case NODE_MCU_AT90CAN128:
            return "AT90CAN128";
       case NODE_MCU_PIC18F46K80:
            return "PIC18F46K80";
    }
    return "UNKNOWN";
}

const char* ExtH9Frame::mcu_f_type_to_string(std::uint8_t mcu_f) {
    switch (mcu_f) {
        case NODE_MCU_F_2MHz:
            return "2 MHz";
        case NODE_MCU_F_4MHz:
            return "4 MHz";
        case NODE_MCU_F_6MHz:
            return "6 MHz";
        case NODE_MCU_F_8MHz:
            return "8 MHz";
        case NODE_MCU_F_12MHz:
            return "12 MHz";
        case NODE_MCU_F_16MHz:
            return "16 MHz";
    }
    return "UNKNOWN";
}

const char* ExtH9Frame::reset_reason_to_string(std::uint8_t reset_reason) {
    switch (reset_reason) {
        case NODE_RESET_BY_UNKNOWN:
            return "UNKNOWN";
        case NODE_RESET_BY_POWER_ON:
            return "power on";
        case NODE_RESET_BY_WATCHDOG:
            return "watchdog";
        case NODE_RESET_BY_BROWN_OUT:
            return "brown out";
        case NODE_RESET_BY_EXTERNAL_SOURCE:
            return "external source";
    }
    return "UNKNOWN";
}
