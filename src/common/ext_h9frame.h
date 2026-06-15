/*
 * H9 project
 *
 * Created by crowx on 2023-09-07.
 *
 * Copyright (C) 2023 Kamil Palkowski. All rights reserved.
 */

#pragma once

#include <chrono>
#include <h9def.h>
#include <h9frame.h>
#include <nlohmann/json.hpp>
#include <string>

#include "types.h"

class ExtH9Frame {
  private:
    h9frame_t _frame;
    std::string _origin;

    unsigned int valid;

    timestamp_t _creation_timestamp;

  public:
    constexpr static unsigned int VALID_ORIGIN = 1 << 0;
    constexpr static unsigned int VALID_TYPE = 1 << 1;
    constexpr static unsigned int VALID_SEQNUM = 1 << 2;
    constexpr static unsigned int VALID_DESTINATION_ID = 1 << 3;
    constexpr static unsigned int VALID_BROADCAST_GROUP = 1 << 4;
    constexpr static unsigned int VALID_SOURCE_ID = 1 << 5;
    constexpr static unsigned int VALID_DLC = 1 << 6;
    constexpr static unsigned int VALID_DATA = 1 << 7;

    constexpr static unsigned int VALID_ALL = (VALID_ORIGIN | VALID_TYPE | VALID_SEQNUM | VALID_DESTINATION_ID | VALID_SOURCE_ID | VALID_DLC | VALID_DATA);
    constexpr static unsigned int VALID_UNUSED = ~VALID_ALL;

    enum class Type : std::uint8_t {
        RES1 = H9FRAME_TYPE_RES1,
        PAGE_START = H9FRAME_TYPE_PAGE_START,
        QUIT_BOOTLOADER = H9FRAME_TYPE_QUIT_BOOTLOADER,
        PAGE_FILL = H9FRAME_TYPE_PAGE_FILL,
        BOOTLOADER_TURNED_ON = H9FRAME_TYPE_BOOTLOADER_TURNED_ON,
        PAGE_FILL_NEXT = H9FRAME_TYPE_PAGE_FILL_NEXT,
        PAGE_WRITED = H9FRAME_TYPE_PAGE_WRITED,
        PAGE_FILL_BREAK = H9FRAME_TYPE_PAGE_FILL_BREAK,
        COMMAND_ERROR = H9FRAME_TYPE_COMMAND_ERROR,
        REG_VALUE = H9FRAME_TYPE_REG_VALUE,
        SET_REG = H9FRAME_TYPE_SET_REG,
        GET_REG = H9FRAME_TYPE_GET_REG,
        SET_BIT = H9FRAME_TYPE_SET_BIT,
        CLEAR_BIT = H9FRAME_TYPE_CLEAR_BIT,
        NODE_UPGRADE = H9FRAME_TYPE_NODE_UPGRADE,
        NODE_RESET = H9FRAME_TYPE_NODE_RESET,
        DISCOVER = H9FRAME_TYPE_DISCOVER,
        GROUP_RESET = H9FRAME_TYPE_GROUP_RESET,
        NODE_FAULT = H9FRAME_TYPE_NODE_FAULT,
        REG_VALUE_BROADCAST = H9FRAME_TYPE_REG_VALUE_BROADCAST,
        NODE_HEARTBEAT = H9FRAME_TYPE_NODE_HEARTBEAT,
        NODE_INFO = H9FRAME_TYPE_NODE_INFO,
        NODE_TURNED_ON = H9FRAME_TYPE_NODE_TURNED_ON,
        RES2 = H9FRAME_TYPE_RES2,
        NODE_SPECIFIC_BROADCAST0 = H9FRAME_TYPE_NODE_SPECIFIC_BROADCAST0,
        NODE_SPECIFIC_BROADCAST1 = H9FRAME_TYPE_NODE_SPECIFIC_BROADCAST1,
        NODE_SPECIFIC_BROADCAST2 = H9FRAME_TYPE_NODE_SPECIFIC_BROADCAST2,
        NODE_SPECIFIC_BROADCAST3 = H9FRAME_TYPE_NODE_SPECIFIC_BROADCAST3,
        NODE_SPECIFIC_BROADCAST4 = H9FRAME_TYPE_NODE_SPECIFIC_BROADCAST4,
        NODE_SPECIFIC_BROADCAST5 = H9FRAME_TYPE_NODE_SPECIFIC_BROADCAST5,
        NODE_SPECIFIC_BROADCAST6 = H9FRAME_TYPE_NODE_SPECIFIC_BROADCAST6,
        NODE_SPECIFIC_BROADCAST7 = H9FRAME_TYPE_NODE_SPECIFIC_BROADCAST7
    };

    enum class Error : std::uint8_t {
        INVALID_FRAME = H9FRAME_ERROR_INVALID_FRAME,
        BOOTLOADER_UNSUPPORTED = H9FRAME_ERROR_BOOTLOADER_UNSUPPORTED,
        UNSUPPORTED_OPERATION = H9FRAME_ERROR_UNSUPPORTED_OPERATION,
        UNSUPPORTED_REGISTER = H9FRAME_ERROR_UNSUPPORTED_REGISTER,
        INVALID_REGISTER = H9FRAME_ERROR_INVALID_REGISTER,
        READ_ONLY_REGISTER = H9FRAME_ERROR_READ_ONLY_REGISTER,
        WRITE_ONLY_REGISTER = H9FRAME_ERROR_WRITE_ONLY_REGISTER,
        REGISTER_SIZE_MISMATCH = H9FRAME_ERROR_REGISTER_SIZE_MISMATCH,
    };

    constexpr static int H9FRAME_DESTINATION_ID_BIT_LENGTH = H9FRAME_ID_BIT_LENGTH;
    constexpr static int H9FRAME_SOURCE_ID_BIT_LENGTH = H9FRAME_ID_BIT_LENGTH;
    constexpr static std::uint16_t BROADCAST_ID = H9FRAME_BROADCAST_ID;
    constexpr static int H9FRAME_TYPE_MAX_VALUE = (1 << H9FRAME_TYPE_BIT_LENGTH) - 1;
    constexpr static int H9FRAME_SEQNUM_MAX_VALUE = (1 << H9FRAME_SEQNUM_BIT_LENGTH) - 1;
    constexpr static int H9FRAME_DESTINATION_ID_MAX_VALUE = (1 << H9FRAME_DESTINATION_ID_BIT_LENGTH) - 1;
    constexpr static int H9FRAME_SOURCE_ID_MAX_VALUE = (1 << H9FRAME_SOURCE_ID_BIT_LENGTH) - 1;
    constexpr static int H9FRAME_DATA_LENGTH = 8;

    template<typename E, typename R = std::underlying_type_t<E>>
    static R to_underlying(E e) noexcept {
        return static_cast<R>(e);
    }

    template<typename E>
    static E from_underlying(std::underlying_type_t<E> e) noexcept {
        return static_cast<E>(e);
    }

    static const char* type_to_string(Type type);
    static const char* error_to_string(Error error);
    static const char* mcu_type_to_string(std::uint8_t mcu);
    static const char* mcu_f_type_to_string(std::uint8_t mcu_f);
    static const char* reset_reason_to_string(std::uint8_t reset_reason);

    ExtH9Frame();
    ExtH9Frame(const h9frame_t& frame, const std::string& origin);
    ExtH9Frame(const std::string& origin, ExtH9Frame::Type type, std::uint16_t dst, std::uint8_t dlc = 0, const std::vector<std::uint8_t>& data = {});

    unsigned int valid_member();
    unsigned int invalid_member();

    timestamp_t creation_timestamp() const { return _creation_timestamp; }

    const h9frame_t& frame() const { return _frame; }

    const std::string& origin() const { return _origin; }

    ExtH9Frame::Type type() const { return ExtH9Frame::from_underlying<ExtH9Frame::Type>(_frame.type); }

    std::uint8_t flags() const { return _frame.unicast.flags; }

    std::uint8_t seqnum() const { return _frame.unicast.seqnum; }

    std::uint8_t destination_id() const { return _frame.unicast.destination_id; }

    std::uint16_t broadcast_group() const { return _frame.broadcast.group; }

    std::uint8_t source_id() const { return _frame.source_id; }

    std::uint8_t dlc() const { return _frame.dlc; }

    const std::uint8_t* data() const { return _frame.data; };

    void origin(const std::string& origin);
    void type(ExtH9Frame::Type type);
    void type(std::uint8_t type);
    void seqnum(std::uint8_t seqnum);
    void destination_id(std::uint8_t destination_id);
    void broadcast_group(std::uint16_t broadcast_group);
    void source_id(std::uint8_t source_id);
    void dlc(std::uint8_t dlc);
    void data(const std::vector<std::uint8_t>& data);

    bool is_unicast() const;
    bool is_broadcast() const;
};

void to_json(nlohmann::json& j, const ExtH9Frame& f);
void from_json(const nlohmann::json& j, ExtH9Frame& f);
