/*
 * H9 project
 *
 * Created by crowx on 2023-09-07.
 *
 * Copyright (C) 2023 Kamil Palkowski. All rights reserved.
 */

#pragma once

#include <chrono>
#include <nlohmann/json.hpp>
#include <string>

namespace {
#include <h9def.h>
}

#include "types.h"

class H9Frame {
  public:
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
        /* --- SPECIAL BROADCAST --- */
        DISCOVER = H9FRAME_TYPE_DISCOVER,
        GROUP_RESET = H9FRAME_TYPE_GROUP_RESET,
        /* --- BROADCAST --- */
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

    enum class Flags : std::uint8_t {
        SINGE_FRAME = H9FRAME_FLAG_SINGE_FRAME,
        MULTI_FRAME_FIRST = H9FRAME_FLAG_MULTI_FRAME_FIRST,
        MULTI_FRAME_MIDDLE = H9FRAME_FLAG_MULTI_FRAME_MIDDLE,
        MULTI_FRAME_LAST = H9FRAME_FLAG_MULTI_FRAME_LAST
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

    constexpr static std::uint16_t BROADCAST_ID = H9FRAME_BROADCAST_ID;

    constexpr static int MAX_DATA_LENGTH = 8;
    constexpr static int SERIALIZATION_LENGTH = 4 + 1 + MAX_DATA_LENGTH;
    constexpr static int TYPE_BIT_LENGTH = H9FRAME_TYPE_BIT_LENGTH;
    constexpr static int FLAGS_BITS_LENGTH = H9FRAME_FLAGS_BITS_LENGTH;
    constexpr static int SEQNUM_BIT_LENGTH = H9FRAME_SEQNUM_BIT_LENGTH;
    constexpr static int ID_BIT_LENGTH = H9FRAME_ID_BIT_LENGTH;
    constexpr static int BROADCAST_GROUP_BIT_LENGTH = H9FRAME_BROADCAST_GROUP_BIT_LENGTH;

    constexpr static int TYPE_MAX_VALUE = (1 << H9FRAME_TYPE_BIT_LENGTH) - 1;
    constexpr static int SEQNUM_MAX_VALUE = (1 << H9FRAME_SEQNUM_BIT_LENGTH) - 1;
    constexpr static int ID_MAX_VALUE = (1 << H9FRAME_ID_BIT_LENGTH) - 1;

  private:
    std::string _origin;
    timestamp_t _creation_timestamp;

    Type _type;
    uint8_t _source_id;
    Flags _flags;
    uint8_t _destination_id;
    uint8_t _seqnum;
    uint16_t _group;
    uint8_t _dlc;
    uint8_t _data[8];

  public:
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

    H9Frame();
    //H9Frame(const std::string& origin, uint32_t can_id, std::uint8_t dlc = 0, const std::array<std::uint8_t, MAX_DATA_LENGTH>& data = {});
    //H9Frame(const std::string& origin, const std::uint8_t serialized_data[SERIALIZATION_LENGTH]);

    H9Frame(const std::string& origin, Type type, Flags flags, std::uint8_t dst, const std::vector<std::uint8_t>& data = {});
    H9Frame(const std::string& origin, Type type, std::uint16_t broadcast_group, const std::vector<std::uint8_t>& data = {});
    // H9Frame(const std::string& origin, H9Frame::Type type, std::uint16_t dst, const std::vector<std::uint8_t>& data = {});

    [[nodiscard]] std::uint32_t can_id() const;
    void can_id(std::uint32_t can_id);

    [[nodiscard]] std::array<uint8_t, SERIALIZATION_LENGTH> serialize() const;
    void deserialize(const std::string& origin, uint32_t can_id, std::uint8_t dlc = 0, const std::vector<std::uint8_t>& data = {});
    void deserialize(const std::string& origin, const std::uint8_t serialized_data[SERIALIZATION_LENGTH]);

    [[nodiscard]] timestamp_t creation_timestamp() const { return _creation_timestamp; }

    [[nodiscard]] const std::string& origin() const { return _origin; }
    [[nodiscard]] H9Frame::Type type() const { return _type; }
    [[nodiscard]] std::uint8_t raw_type() const { return to_underlying(_type); }
    [[nodiscard]] std::uint8_t source_id() const { return _source_id; }
    [[nodiscard]] std::uint8_t seqnum() const { return _seqnum; }
    [[nodiscard]] std::uint8_t destination_id() const { return _destination_id; }
    [[nodiscard]] Flags flags() const { return _flags; }
    [[nodiscard]] std::uint8_t raw_flags() const { return to_underlying(_flags); }
    [[nodiscard]] std::uint16_t broadcast_group() const { return _group; }
    [[nodiscard]] std::uint8_t dlc() const { return _dlc; }
    [[nodiscard]] const std::uint8_t* data() const { return _data; }

    void origin(const std::string& origin);
    void type(H9Frame::Type type);
    void type(std::uint8_t type);
    void source_id(std::uint8_t source_id);
    void seqnum(std::uint8_t seqnum);
    void destination_id(std::uint8_t destination_id);
    void flags(Flags flags);
    void flags(std::uint8_t  flags);
    void broadcast_group(std::uint16_t broadcast_group);

    void dlc(std::uint8_t dlc);
    void data(const std::vector<std::uint8_t>& data);
    void data(const std::uint8_t data[MAX_DATA_LENGTH]);
    uint8_t* data_raw();
    //void data(const std::array<std::uint8_t, MAX_DATA_LENGTH>& data);

    bool is_unicast() const;
    bool is_broadcast() const;

    bool is_valid() const;
};

void to_json(nlohmann::json& j, const H9Frame& f);
void from_json(const nlohmann::json& j, H9Frame& f);
