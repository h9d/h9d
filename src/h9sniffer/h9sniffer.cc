/*
 * H9 project
 *
 * Created by SQ8KFH on 2019-04-09.
 *
 * Copyright (C) 2019-2023 Kamil Palkowski. All rights reserved.
 */

#include "config.h"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>

#include "h9_frame.h"
#include "h9_configurator.h"
#include "h9connector.h"
#include "node_desc_loader.h"

namespace {

class H9SnifferConfigurator: public H9Configurator {
  private:
    void add_app_specific_opt() {
        // clang-format off
        options.add_options("")
                ("e,extended", "Extended output")
                ("s,simple", "Simple output")
                ("n,nodes-desc", "Nodes description file", cxxopts::value<std::string>())
                ;
        // clang-format on
    }

    void parse_app_specific_opt(const cxxopts::ParseResult& result) {
        extended = result.count("extended");
        simple = result.count("simple");
        if (result.count("nodes-desc"))
            node_description_file = result["nodes-desc"].as<std::string>();
    }

  public:
    bool extended;
    bool simple;
    using H9Configurator::node_description_file;

    H9SnifferConfigurator():
        H9Configurator("h9sniffer", "The H9 bus frames sniffer.") {}
};

class NodeRegistry {
    std::map<std::uint8_t, std::uint16_t> _node_id_to_type;
  public:
    void update(const H9Frame& frame) {
        if (frame.type() == H9Frame::Type::NODE_TURNED_ON ||
            frame.type() == H9Frame::Type::NODE_INFO) {
            std::uint16_t node_type = (frame.data()[0] << 8) | frame.data()[1];
            _node_id_to_type[frame.source_id()] = node_type;
        }
    }

    const NodeDescLoader::NodeDesc* get_node_desc(std::uint8_t node_id, const NodeDescLoader& loader) const {
        auto it = _node_id_to_type.find(node_id);
        if (it == _node_id_to_type.end())
            return nullptr;
        return loader.find_by_type(it->second);
    }

    const NodeDescLoader::RegisterDesc* get_register_desc(std::uint8_t node_id, std::uint8_t reg_num, const NodeDescLoader& loader) const {
        auto it = _node_id_to_type.find(node_id);
        if (it == _node_id_to_type.end())
            return loader.find_register(H9Frame::BROADCAST_ALL_GROUP, reg_num); //dajemy nie istniejeacy node id tylko zeby dostac potencjalne staandard req ktore sa takie sama dla wszystkich
        return loader.find_register(it->second, reg_num);
    }
};

} // namespace

void print_reg_value(const H9Frame& frame, const NodeDescLoader::RegisterDesc* reg_desc) {
    bool multi_frame = frame.flags() != H9Frame::Flags::SINGE_FRAME;
    int multi_frame_offset = multi_frame ? 1 : 0;

    if (frame.dlc() <= 1 + multi_frame_offset)
        return;

    const std::uint8_t* d = frame.data();
    int data_len = frame.dlc() - 1 - multi_frame_offset;

    if (multi_frame && frame.flags() == H9Frame::Flags::MULTI_FRAME_FIRST) {
        fmt::print("    total frame: {}\n", d[1]);
        fmt::print("    frame offset: {}\n", 0);
    }
    else if (multi_frame && frame.flags() == H9Frame::Flags::MULTI_FRAME_LAST) {
        fmt::print("    frame offset: {} (LAST)\n", d[1]);
    }
    else if (multi_frame) {
        fmt::print("    frame offset: {}\n", d[1]);
    }

    // bool bitfield: size is number of bits, bits_names[i] = name of bit i
    if (reg_desc && reg_desc->type == "bool" && reg_desc->size > 1) {
        int num_bits = reg_desc->size;
        int num_bytes = (num_bits + 7) / 8;
        std::uint32_t bval = 0;

        for (int i = 0; i < num_bytes && i < data_len; ++i)
            bval = (bval << 8) | d[1 + i + multi_frame_offset];

        for (int i = num_bits - 1; i >= 0; --i) {
            int bit_val = (bval >> i) & 1;
            if (i < static_cast<int>(reg_desc->bits_names.size()))
                fmt::print("      bit {:2d} {:<24s}: {}\n", i, reg_desc->bits_names[i], bit_val);
            else
                fmt::print("      bit {:2d}: {}\n", i, bit_val);
        }
        return;
    }

    fmt::print("    value: ");
    switch (data_len) {
        case 1: std::cout << static_cast<unsigned int>(d[1 + multi_frame_offset]); break;
        case 2: std::cout << static_cast<unsigned int>((d[1 + multi_frame_offset] << 8) | d[2 + multi_frame_offset]); break;
        case 4: std::cout << static_cast<unsigned int>((d[1 + multi_frame_offset] << 24) | (d[2 + multi_frame_offset] << 16) | (d[3 + multi_frame_offset] << 8) | d[4 + multi_frame_offset]); break;
        default: break;
    }

    if (reg_desc) {
        if (reg_desc->type == "uint") {
            std::uint32_t uval = 0;
            for (int i = 0; i < data_len && i < 4; ++i)
                uval = (uval << 8) | d[1 + multi_frame_offset + i];
            fmt::print(" (uint{}: {})", reg_desc->size, uval);
        } else if (reg_desc->type == "bool") {
            fmt::print(" (bool: {})", d[1 + multi_frame_offset] ? "true" : "false");
        } else if (reg_desc->type == "int") {
            std::int32_t ival = 0;
            if (data_len >= 1) ival = static_cast<std::int8_t>(d[1 + multi_frame_offset]);
            if (data_len >= 2) ival = static_cast<std::int16_t>((d[1 + multi_frame_offset] << 8) | d[2 + multi_frame_offset]);
            if (data_len >= 4) ival = static_cast<std::int32_t>((d[1 + multi_frame_offset] << 24) | (d[2 + multi_frame_offset] << 16) | (d[3 + multi_frame_offset] << 8) | d[4 + multi_frame_offset]);
            fmt::print(" (int{}: {})", reg_desc->size, ival);
        } else if (reg_desc->type == "char") {
            fmt::print(" (char: '{}')", static_cast<char>(d[1 + multi_frame_offset]));
        } else if (reg_desc->type == "str") {
            int slen = std::min(data_len, 7);
            fmt::print(" (str: \"");
            for (int i = 0; i < slen && d[1 + multi_frame_offset + i] != 0; ++i)
                fmt::print("{}", static_cast<char>(d[1 + multi_frame_offset + i]));
            fmt::print("\")");
        } else if (reg_desc->type == "float") {
            //TODO: poprawic do floatow 4 bajtowych i kopiowanie memory moze byc nie zgodne z kolejnoscia bajtow architektury
            float fval = 0.0f;
            std::memcpy(&fval, &d[1 + multi_frame_offset], sizeof(float));
            fmt::print(" (float: {:.4f})", fval);
        }
    }

    std::cout << std::endl;
}

void print_frame(const H9Frame& frame, const NodeRegistry& registry, const NodeDescLoader& loader) {
    H9Frame::Type type = frame.type();
    if (const auto* desc = registry.get_node_desc(frame.source_id(), loader)) {
        fmt::print("    node type name: {}\n", desc->name);
    }
    std::cout << "    frame type: " << H9Frame::type_to_string(type) << std::endl;

    auto print_reg_label = [&](std::uint8_t node_id, std::uint8_t reg_num) -> const NodeDescLoader::RegisterDesc* {
        const auto* reg_desc = registry.get_register_desc(node_id, reg_num, loader);
        if (reg_desc && !reg_desc->name.empty())
            fmt::print("    reg: {:d} (name: {})\n", reg_num, reg_desc->name);
        else
            fmt::print("    reg: {:d}\n", reg_num);
        return reg_desc;
    };

    if (type == H9Frame::Type::REG_VALUE || type == H9Frame::Type::REG_VALUE_BROADCAST) {
        const auto* reg_desc = print_reg_label(frame.source_id(), frame.data()[0]);
        print_reg_value(frame, reg_desc);
    } else if (type == H9Frame::Type::SET_REG || type == H9Frame::Type::GET_REG) {
        const auto* reg_desc = print_reg_label(frame.destination_id(), frame.data()[0]);
        print_reg_value(frame, reg_desc);
    } else if (type == H9Frame::Type::SET_BIT) {
        print_reg_label(frame.destination_id(), frame.data()[0]);
        std::cout << "    set bit: " << static_cast<unsigned int>(frame.data()[1]) << std::endl;
    } else if (type == H9Frame::Type::CLEAR_BIT) {
        print_reg_label(frame.destination_id(), frame.data()[0]);
        std::cout << "    clear bit: " << static_cast<unsigned int>(frame.data()[1]) << std::endl;
    } else if (type == H9Frame::Type::NODE_TURNED_ON || type == H9Frame::Type::NODE_INFO) {
        fmt::print("    node type: {:d}\n", (frame.data()[0] << 8 | frame.data()[1]));
        fmt::print("    node firmware: {:d}.{:d}{:c}\n", (frame.data()[2] << 8 | frame.data()[3]), (frame.data()[4] << 8 | frame.data()[5]), frame.data()[6]);
        fmt::print("    node reset_reason {:d} ({})\n", frame.data()[7], H9Frame::reset_reason_to_string(frame.data()[7]));
    } else if (type == H9Frame::Type::BOOTLOADER_TURNED_ON) {
        fmt::print("    node type: {:d}\n", (frame.data()[0] << 8 | frame.data()[1]));
        fmt::print("    bootloader firmware: {:d}.{:d}\n", (frame.data()[2] << 8 | frame.data()[3]), (frame.data()[4] << 8 | frame.data()[5]));
        fmt::print("    node mcu: {:d} ({})\n", frame.data()[6], H9Frame::mcu_type_to_string(frame.data()[6]));
        fmt::print("    mcu F: {:d} ({})\n", frame.data()[7], H9Frame::mcu_f_type_to_string(frame.data()[7]));
    } else if (type == H9Frame::Type::COMMAND_ERROR) {
        int err_num = static_cast<int>(frame.data()[0]);
        std::cout << "    error: " << err_num << " - " << H9Frame::error_to_string(H9Frame::from_underlying<H9Frame::Error>(err_num)) << std::endl;
    }
    else if (type == H9Frame::Type::NODE_SPECIFIC_BROADCAST6 && frame.broadcast_group() == 6) {
        fmt::print("    atu ref: {:d}\n", (frame.data()[0] << 8 | frame.data()[1]));
        fmt::print("    atu fwd: {:d}\n", (frame.data()[2] << 8 | frame.data()[3]));
        fmt::print("    freq: {:d}\n", (frame.data()[4] << 8 | frame.data()[5]));
        fmt::print("    swr: {:.2f}\n", (frame.data()[6] << 8 | frame.data()[7]) / 100.0f);
    }
    else if (type == H9Frame::Type::NODE_SPECIFIC_BROADCAST7 && frame.broadcast_group() == 6) {
        fmt::print("    atu ref: {:d}\n", (frame.data()[0] << 8 | frame.data()[1]));
        fmt::print("    atu fwd: {:d}\n", (frame.data()[2] << 8 | frame.data()[3]));
        fmt::print("    freq: {:d}\n", (frame.data()[4] << 8 | frame.data()[5]));
        fmt::print("    l: {:d}\n", frame.data()[6]);
        fmt::print("    c: {:d}\n", frame.data()[7]);
    }
}

int main(int argc, char** argv) {
    H9SnifferConfigurator h9;
    h9.logger_initial_setup();
    h9.parse_command_line_arg(argc, argv);
    h9.logger_setup();
    h9.load_configuration();

    NodeDescLoader node_desc_loader;
    if (h9.extended && !h9.node_description_file.empty()) {
        node_desc_loader.load_file(h9.node_description_file);
    }
    NodeRegistry node_registry;

    if (h9.extended && h9.get_scheme() == h9.H9D_SCHEME) {
        H9Connector& h9_connector = h9.get_connector();
        h9_connector.connect(h9.get_userinfo());
    }

    std::unique_ptr<BusDriver> bus;

    try {
        bus = h9.get_bus_driver();
        bus->open();
    } catch (std::invalid_argument& e) {
        SPDLOG_ERROR("Can not connect to bus: {}.", e.what());
        exit(EXIT_FAILURE);
    } catch (std::system_error& e) {
        SPDLOG_ERROR("Can not connect to bus: {}.", e.code().message());
        exit(EXIT_FAILURE);
    } catch (std::runtime_error& e) {
        SPDLOG_ERROR("Can not connect to bus: {}.", e.what());
        exit(EXIT_FAILURE);
    }

    int output = 1;

    if (h9.simple && !h9.extended) {
        output = 0;
        std::cout << "type,source_id,destination_id,flags,seqnum,broadcast_group,dlc,data\n";
    }
    if (h9.extended && !h9.simple) {
        output = 2;
    }


    while (true) {
        H9Frame frame;
        try {
            if (bus->recv_frame(frame) <= BusDriver::SOCKET_CLOSE) {
                continue;
            }
        } catch (std::system_error& e) {
            SPDLOG_ERROR("Messages receiving error: {}.", e.code().message());
            exit(EXIT_FAILURE);
        } catch (std::runtime_error& e) {
            SPDLOG_ERROR("Messages receiving error: {}.", e.what());
            exit(EXIT_FAILURE);
        }

        node_registry.update(frame);

        if (output == 0) {
            if (frame.is_unicast())
                fmt::print("{:d},{:d},{:d},{:d},{:d},,{:d},", H9Frame::to_underlying(frame.type()), frame.source_id(), frame.destination_id(), H9Frame::to_underlying(frame.flags()), frame.seqnum(), frame.dlc());
            else
                fmt::print("{:d},{:d},,,,{:d},{:d},", H9Frame::to_underlying(frame.type()), frame.source_id(), frame.broadcast_group(), frame.dlc());
            for (int i = 0; i < frame.dlc(); ++i) {
                fmt::print("{:02X}", frame.data()[i]);
            }
            fmt::print("\n");
        } else {
            if (frame.is_unicast()) {
                fmt::print("{:d} -> {:d} type: {:d} ({}) flags: {} seqnum: {:d} dlc: {:d} data: ", frame.source_id(), frame.destination_id(),
                    static_cast<unsigned int>(H9Frame::to_underlying(frame.type())),
                    H9Frame::type_to_string(frame.type()),
                    H9Frame::to_underlying(frame.flags()),
                    frame.seqnum(),
                    frame.dlc());
            }
            else {
                fmt::print("{:d} -> [{:d}] type: {:d} ({}) dlc: {:d} data: ", frame.source_id(), frame.broadcast_group(),
                    static_cast<unsigned int>(H9Frame::to_underlying(frame.type())),
                    H9Frame::type_to_string(frame.type()),
                    frame.dlc());
            }
            for (int i = 0; i < frame.dlc(); ++i) {
                fmt::print("{:02X} ", frame.data()[i]);
            }
            fmt::print("\n");

            if (output == 2) {
                print_frame(frame, node_registry, node_desc_loader);
            }
        }
    }
    return EXIT_SUCCESS;
}
