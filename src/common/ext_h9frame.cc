/*
 * H9 project
 *
 * Created by crowx on 2023-09-07.
 *
 * Copyright (C) 2023 Kamil Palkowski. All rights reserved.
 */

#include "ext_h9frame.h"

ExtH9Frame::ExtH9Frame():
    _creation_timestamp(timestamp_t::clock::now()) {
}

// ExtH9Frame::ExtH9Frame(const std::string& origin, uint32_t can_id, std::uint8_t dlc, const std::array<std::uint8_t, 8>& data):
//     _origin(origin),
//     _creation_timestamp(timestamp_t::clock::now()) {
//
//     this->can_id(can_id);
//
//     this->dlc(dlc);
//     this->data(data);
// }
//
// ExtH9Frame::ExtH9Frame(const std::string& origin, const std::uint8_t serialized_data[SERIALIZATION_LENGTH]):
//     _origin(origin),
//     _creation_timestamp(timestamp_t::clock::now()) {
//
//     _type = from_underlying<ExtH9Frame::Type>(serialized_data[0] & ((1 << H9FRAME_TYPE_BIT_LENGTH) - 1));
//     _source_id = serialized_data[1] & ((1 << H9FRAME_ID_BIT_LENGTH) - 1);
//
//     if (is_unicast()) {
//         _flags = from_underlying<ExtH9Frame::Flags>((serialized_data[2] >> (8 - H9FRAME_FLAGS_BITS_LENGTH)) & ((1 << H9FRAME_FLAGS_BITS_LENGTH) - 1));
//         _destination_id = (serialized_data[2] << H9FRAME_FLAGS_BITS_LENGTH) | (serialized_data[3] >> H9FRAME_SEQNUM_BIT_LENGTH);
//         _seqnum = serialized_data[3] & ((1 << H9FRAME_SEQNUM_BIT_LENGTH) - 1);
//     }
//     else {
//         _group = static_cast<std::uint16_t>(serialized_data[2]) << 8 | static_cast<std::uint16_t>(serialized_data[3]);
//     }
//
//     _dlc = serialized_data[4] < 8 ? serialized_data[4] : 8;
//
//     for (int i = 0; i < 8; ++i) {
//         _data[i] = serialized_data[5 + i];
//     }
// }

ExtH9Frame::ExtH9Frame(const std::string& origin, ExtH9Frame::Type type, ExtH9Frame::Flags flags, std::uint8_t dst, const std::vector<std::uint8_t>& data):
    _creation_timestamp(timestamp_t::clock::now()) {
    this->origin(origin);
    this->type(type);
    this->flags(flags);
    this->destination_id(dst);
    this->data(data);

    assert(type < ExtH9Frame::Type::DISCOVER);
}

ExtH9Frame::ExtH9Frame(const std::string& origin, Type type, std::uint16_t broadcast_group, const std::vector<std::uint8_t>& data):
_creation_timestamp(timestamp_t::clock::now()) {
    this->origin(origin);
    this->type(type);
    this->broadcast_group(broadcast_group);
    this->data(data);

    assert(type >= ExtH9Frame::Type::DISCOVER);
}

 std::uint32_t ExtH9Frame::can_id() const {
    uint32_t id = 0;
    id |= to_underlying(_type)& ((1 << TYPE_BIT_LENGTH) - 1);
    id <<= ID_BIT_LENGTH;
    id |= _source_id & ((1 << ID_BIT_LENGTH) - 1);
    if (is_unicast()) {
        id <<= FLAGS_BITS_LENGTH;
        id |= to_underlying(_flags) & ((1 << FLAGS_BITS_LENGTH) - 1);
        id <<= ID_BIT_LENGTH;
        id |= _destination_id & ((1 << ID_BIT_LENGTH) - 1);
        id <<= SEQNUM_BIT_LENGTH;
        id |= _seqnum & ((1 << SEQNUM_BIT_LENGTH) - 1);
    }
    else {
        id <<= BROADCAST_GROUP_BIT_LENGTH;
        id |= _group & ((1 << BROADCAST_GROUP_BIT_LENGTH) - 1);
    }
    return id;
}

void ExtH9Frame::can_id(std::uint32_t can_id) {
    _type = from_underlying<ExtH9Frame::Type>((uint8_t)((can_id >> (ID_BIT_LENGTH + FLAGS_BITS_LENGTH + ID_BIT_LENGTH + SEQNUM_BIT_LENGTH)) & ((1 << TYPE_BIT_LENGTH) - 1)));
    _source_id = static_cast<std::uint8_t>((can_id >> (FLAGS_BITS_LENGTH + ID_BIT_LENGTH + SEQNUM_BIT_LENGTH)) & ((1 << ID_BIT_LENGTH) - 1));

    if (is_unicast()) {
        _flags = from_underlying<ExtH9Frame::Flags>((uint8_t)((can_id >> (ID_BIT_LENGTH + SEQNUM_BIT_LENGTH)) & ((1 << FLAGS_BITS_LENGTH) - 1)));
        _destination_id = static_cast<std::uint8_t>((can_id >> SEQNUM_BIT_LENGTH) & ((1 << ID_BIT_LENGTH) - 1));
        _seqnum = static_cast<std::uint8_t>((can_id >> 0) & ((1 << SEQNUM_BIT_LENGTH) - 1));
    }
    else {
        _group = static_cast<std::uint16_t>(can_id & ((1 << BROADCAST_GROUP_BIT_LENGTH) - 1));
    }
}

std::array<uint8_t, ExtH9Frame::SERIALIZATION_LENGTH> ExtH9Frame::serialize() const {
    std::array<uint8_t, SERIALIZATION_LENGTH> ret;

    uint32_t id = can_id();

    ret[0] = id >> 24 & 0xff;
    ret[1] = id >> 16 & 0xff;
    ret[2] = id >> 8 & 0xff;
    ret[3] = id & 0xff;

    ret[4] = _dlc;

    for (int i = 0; i < 8; ++i) {
        ret[5 + i] = _data[i];
    }

    return std::move(ret);
}

void ExtH9Frame::deserialize(const std::string& origin, uint32_t can_id, std::uint8_t dlc, const std::vector<std::uint8_t>& data) {
    _origin = origin;

    this->can_id(can_id);
    this->data(data);
    this->dlc(dlc);
}

void ExtH9Frame::deserialize(const std::string& origin, const std::uint8_t serialized_data[SERIALIZATION_LENGTH]) {
    _origin = origin;

    _type = from_underlying<ExtH9Frame::Type>(serialized_data[0] & ((1 << TYPE_BIT_LENGTH) - 1));
    _source_id = serialized_data[1] & ((1 << ID_BIT_LENGTH) - 1);

    if (is_unicast()) {
        _flags = from_underlying<ExtH9Frame::Flags>((serialized_data[2] >> (8 - FLAGS_BITS_LENGTH)) & ((1 << FLAGS_BITS_LENGTH) - 1));
        _destination_id = (serialized_data[2] << FLAGS_BITS_LENGTH) | (serialized_data[3] >> SEQNUM_BIT_LENGTH);
        _seqnum = serialized_data[3] & ((1 << SEQNUM_BIT_LENGTH) - 1);
    }
    else {
        _group = static_cast<std::uint16_t>(serialized_data[2]) << 8 | static_cast<std::uint16_t>(serialized_data[3]);
    }

    _dlc = serialized_data[4] < 8 ? serialized_data[4] : 8;

    for (int i = 0; i < 8; ++i) {
        _data[i] = serialized_data[5 + i];
    }
}

void ExtH9Frame::origin(const std::string& origin) {
    _origin = origin;
}

void ExtH9Frame::type(ExtH9Frame::Type type) {
    _type = type;
}

void ExtH9Frame::type(std::uint8_t type) {
    _type = ExtH9Frame::from_underlying<ExtH9Frame::Type>(type);
}

void ExtH9Frame::seqnum(std::uint8_t seqnum) {
    _seqnum = seqnum;
}

void ExtH9Frame::destination_id(std::uint8_t destination_id) {
    _destination_id = destination_id;
}

void ExtH9Frame::flags(Flags flags) {
    _flags = flags;
}

void ExtH9Frame::flags(std::uint8_t flags) {
    _flags = from_underlying<ExtH9Frame::Flags>(flags);
}

void ExtH9Frame::broadcast_group(std::uint16_t broadcast_group) {
    _group = broadcast_group;
}

void ExtH9Frame::source_id(std::uint8_t source_id) {
    _source_id = source_id;
}

void ExtH9Frame::dlc(std::uint8_t dlc) {
    _dlc = dlc;
}

void ExtH9Frame::data(const std::vector<std::uint8_t>& data) {
    int i = 0;
    for (auto& d : data) {
        _data[i] = d;
        ++i;
    }

    _dlc = i;
}

void ExtH9Frame::data(const std::uint8_t data[MAX_DATA_LENGTH]) {
    for (int i = 0; i < MAX_DATA_LENGTH; ++i) {
        _data[i] = data[i];
    }
}

uint8_t* ExtH9Frame::data_raw() {
    return _data;
}

// void ExtH9Frame::data(const std::array<std::uint8_t, MAX_DATA_LENGTH>& data) {
//     int i = 0;
//     for (auto& d : data) {
//         _data[i] = d;
//         ++i;
//     }
// }

bool ExtH9Frame::is_unicast() const {
    return _type < Type::DISCOVER;
}

bool ExtH9Frame::is_broadcast() const {
    return _type >= Type::DISCOVER;
}

bool ExtH9Frame::is_valid() const {
    //TODO: zrobic cos normalnego
    return true;
}

void to_json(nlohmann::json& j, const ExtH9Frame& f) {
    std::vector<std::uint8_t> data(f.data(), f.data() + f.dlc());
    if (f.is_unicast()) {
        j = nlohmann::json{{"origin", f.origin()},
                       {"type", ExtH9Frame::to_underlying(f.type())},
                       {"source_id", f.source_id()},
                       {"seqnum", f.seqnum()},
                       {"destination_id", f.destination_id()},
                       {"flags", f.flags()},
                       {"dlc", f.dlc()},
                       {"data", data}};
    }
    else {
        j = nlohmann::json{{"origin", f.origin()},
                       {"type", ExtH9Frame::to_underlying(f.type())},
                       {"source_id", f.source_id()},
                       {"broadcast_group", f.broadcast_group()},
                       {"dlc", f.dlc()},
                       {"data", data}};
    }
}

void from_json(const nlohmann::json& j, ExtH9Frame& f) {
    // if (j.count("origin"))
    //     f.origin(j.at("origin").get<std::string>());
    // else
    //     f.origin("");

    if (j.count("type"))
        f.type(j.at("type").get<std::uint8_t>());
    if (j.count("source_id"))
        f.source_id(j.at("source_id").get<std::uint8_t>());

    if (j.count("seqnum"))
        f.seqnum(j.at("seqnum").get<std::uint8_t>());
    if (j.count("destination_id"))
        f.destination_id(j.at("destination_id").get<std::uint8_t>());
    if (j.count("flags"))
        f.flags(j.at("flags").get<std::uint8_t>());

    if (j.count("broadcast_group"))
        f.broadcast_group(j.at("broadcast_group").get<std::uint16_t>());

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
