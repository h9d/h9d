/*
 * H9 project
 *
 * Created by SQ8KFH on 2019-04-09.
 *
 * Copyright (C) 2019-2023 Kamil Palkowski. All rights reserved.
 */

#include "config.h"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <spdlog/spdlog.h>

#include "ext_h9frame.h"
#include "h9_configurator.h"
#include "h9connector.h"

namespace {

class H9SnifferConfigurator: public H9Configurator {
  private:
    void add_app_specific_opt() {
        // clang-format off
        options.add_options("")
                ("e,extended", "Extended output")
                ("s,simple", "Simple output")
                ;
        // clang-format on
    }

    void parse_app_specific_opt(const cxxopts::ParseResult& result) {
        extended = result.count("extended");
        simple = result.count("simple");
    }

  public:
    bool extended;
    bool simple;

    H9SnifferConfigurator():
        H9Configurator("h9sniffer", "The H9 bus packets sniffer.") {}
};

} // namespace

void print_reg_value(const h9frame_t& frame) {
    if (frame.dlc > 1) {
        std::cout << "    value: ";
        switch (frame.dlc) {
        case 2:
            std::cout << static_cast<int>(frame.data[1]);
            break;
        case 3:
            std::cout << static_cast<int>((frame.data[1] << 8) | frame.data[2]);
            break;
        case 5:
            std::cout << static_cast<int>((frame.data[1] << 24) | (frame.data[2] << 16) | (frame.data[3] << 8) | frame.data[4]);
            break;
        }
        char buf[8] = {'\0'};
        for (int i = 0; i < frame.dlc - 1; ++i) {
            if (isprint(frame.data[i + 1])) {
                buf[i] = frame.data[i + 1];
            } else {
                buf[i] = '\0';
                break;
            }
        }
        std::cout << " '" << buf << "'" << std::endl;
    }
}

void print_frame(const h9frame_t& frame) {
    ExtH9Frame::Type type = ExtH9Frame::from_underlying<ExtH9Frame::Type>(frame.type);
    std::cout << "    type name: " << ExtH9Frame::type_to_string(type) << std::endl;
    if (frame.unicast.destination_id == ExtH9Frame::BROADCAST_ID)
        std::cout << "    destination: BROADCAST\n";
    if (type == ExtH9Frame::Type::REG_VALUE ||
        type == ExtH9Frame::Type::REG_VALUE_BROADCAST ||
        type == ExtH9Frame::Type::SET_REG ||
        type == ExtH9Frame::Type::GET_REG) {

        std::cout << "    reg: " << static_cast<unsigned int>(frame.data[0]) << std::endl;
        print_reg_value(frame);
    } else if (type == ExtH9Frame::Type::SET_BIT) {
        std::cout << "    reg: " << static_cast<unsigned int>(frame.data[0]) << std::endl;
        std::cout << "    set bit: " << static_cast<unsigned int>(frame.data[1]) << std::endl;
    } else if (type == ExtH9Frame::Type::CLEAR_BIT) {
        std::cout << "    reg: " << static_cast<unsigned int>(frame.data[0]) << std::endl;
        std::cout << "    clear bit: " << static_cast<unsigned int>(frame.data[1]) << std::endl;
    } else if (type == ExtH9Frame::Type::NODE_TURNED_ON || type == ExtH9Frame::Type::NODE_INFO) {
        fmt::print("    node type: {:d}\n", (frame.data[0] << 8 | frame.data[1]));
        fmt::print("    node firmware: {:d}.{:d}{:c}\n", (frame.data[2] << 8 | frame.data[3]), (frame.data[4] << 8 | frame.data[5]), frame.data[6]);
        fmt::print("    node reset_reason {:d} ({})\n", frame.data[7], ExtH9Frame::reset_reason_to_string(frame.data[7]));
    } else if (type == ExtH9Frame::Type::BOOTLOADER_TURNED_ON) {
        fmt::print("    node type: {:d}\n", (frame.data[0] << 8 | frame.data[1]));
        fmt::print("    bootloader firmware: {:d}.{:d}\n", (frame.data[2] << 8 | frame.data[3]), (frame.data[4] << 8 | frame.data[5]));
        fmt::print("    node mcu: {:d} ({})\n", frame.data[6], ExtH9Frame::mcu_type_to_string(frame.data[6]));
        fmt::print("    mcu F: {:d} ({})\n", frame.data[7], ExtH9Frame::mcu_f_type_to_string(frame.data[7]));
    } else if (type == ExtH9Frame::Type::COMMAND_ERROR) {
        int err_num = static_cast<int>(frame.data[0]);
        std::cout << "    error: " << err_num << " - " << ExtH9Frame::error_to_string(ExtH9Frame::from_underlying<ExtH9Frame::Error>(err_num)) << std::endl;
    }
    else if (type == ExtH9Frame::Type::NODE_SPECIFIC_BROADCAST0 && frame.broadcast.group == 6) {
        fmt::print("    atu ref: {:d}\n", (frame.data[0] << 8 | frame.data[1]));
        fmt::print("    atu fwd: {:d}\n", (frame.data[2] << 8 | frame.data[3]));
        fmt::print("    swr: {:.2f}\n", (frame.data[4] << 8 | frame.data[5]) / 1000.0f);
        fmt::print("    freq: {:d}\n", (frame.data[6] << 8 | frame.data[7]));
    }
}

int main(int argc, char** argv) {
    H9SnifferConfigurator h9;
    h9.logger_initial_setup();
    h9.parse_command_line_arg(argc, argv);
    h9.logger_setup();
    h9.load_configuration();

    std::unique_ptr<BusDriver> bus = h9.get_bus_driver();

    try {
        bus->open();
    } catch (std::system_error& e) {
        SPDLOG_ERROR("Can not connect to h9bus {}:{}: {}.", h9.get_host(), h9.get_port(), e.code().message());
        exit(EXIT_FAILURE);
    } catch (std::runtime_error& e) {
        SPDLOG_ERROR("Can not connect to h9bus {}:{}: {}.", h9.get_host(), h9.get_port(), e.what());
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
        ExtH9Frame frame;
        try {
            if (bus->recv_frame(frame) <= BusDriver::SOCKET_CLOSE)
                continue;
        } catch (std::system_error& e) {
            SPDLOG_ERROR("Messages receiving error: {}.", e.code().message());
            exit(EXIT_FAILURE);
        } catch (std::runtime_error& e) {
            SPDLOG_ERROR("Messages receiving error: {}.", e.what());
            exit(EXIT_FAILURE);
        }

        if (output == 0) {
            if (frame.is_unicast())
                fmt::print("{:d},{:d},{:d},{:d},{:d},,{:d},", ExtH9Frame::to_underlying(frame.type()), frame.source_id(), frame.destination_id(), frame.flags(), frame.seqnum(), frame.dlc());
            else
                fmt::print("{:d},{:d},,,,{:d},{:d},", ExtH9Frame::to_underlying(frame.type()), frame.source_id(), frame.broadcast_group(), frame.dlc());
            for (int i = 0; i < frame.dlc(); ++i) {
                fmt::print("{:02X}", frame.data()[i]);
            }
            fmt::print("\n");
        } else {
            if (frame.is_unicast()) {
                fmt::print("{:d} -> {:d} type: {:d} ({}) seqnum: {:d} dlc: {:d} data: ", frame.source_id(), frame.destination_id(),
                    static_cast<unsigned int>(ExtH9Frame::to_underlying(frame.type())),
                    ExtH9Frame::type_to_string(frame.type()),
                    frame.seqnum(),
                    frame.dlc());
            }
            else {
                fmt::print("{:d} -> [{:d}] type: {:d} ({}) dlc: {:d} data: ", frame.source_id(), frame.broadcast_group(),
                    static_cast<unsigned int>(ExtH9Frame::to_underlying(frame.type())),
                    ExtH9Frame::type_to_string(frame.type()),
                    frame.dlc());
            }
            for (int i = 0; i < frame.dlc(); ++i) {
                fmt::print("{:02X} ", frame.data()[i]);
            }
            fmt::print("\n");

            if (output == 2) {
                print_frame(frame.frame());
            }
        }
    }
    return EXIT_SUCCESS;
}
