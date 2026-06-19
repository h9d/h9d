/*
 * H9 project
 *
 * Created by SQ8KFH on 2019-04-09.
 *
 * Copyright (C) 2019-2023 Kamil Palkowski. All rights reserved.
 */

#include "config.h"

#include <cstdlib>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <unistd.h>

#include "h9_frame.h"
#include "h9_configurator.h"
#include "h9connector.h"

namespace {

class H9SendConfigurator: public H9Configurator {
  private:
    void add_app_specific_opt() {
        // clang-format off
        options.add_options("")
                ("s,src_id", "Source id, if set, a raw frame will be sent", cxxopts::value<std::uint8_t>())
                ("t,type", "Frame type", cxxopts::value<std::underlying_type_t<H9Frame::Type>>())
                ("f,flags", "Flags", cxxopts::value<std::uint8_t>())
                ("d,dst_id", "Destination id", cxxopts::value<std::uint8_t>())
                ("S,seqnum", "Seqnum, if set, a raw frame will be sent", cxxopts::value<std::uint8_t>())
                ("g,group", "Broadcast group", cxxopts::value<std::uint16_t>())
                ("r,repeat", "Repeat the frame every given time in seconds", cxxopts::value<unsigned int>())
                ;
        // clang-format on
        options.parse_positional("data");
        options.positional_help("data");

        options.add_options()("data", "[hex data]", cxxopts::value<std::vector<std::string>>());
    }

  public:
    H9SendConfigurator():
        H9Configurator("h9send", "Sends frames to the H9 Bus.") {}
};

} // namespace

int main(int argc, char* argv[]) {
    H9SendConfigurator h9;
    h9.logger_initial_setup();
    cxxopts::ParseResult res = h9.parse_command_line_arg(argc, argv);
    h9.logger_setup();
    h9.load_configuration();

    H9Frame frame;
    bool raw = false;

    if (res.count("src_id")) {
        frame.source_id(res["src_id"].as<std::uint8_t>());
        raw = true;
    }
    else {
        frame.source_id(h9.get_default_source_id());
    }

    if (res.count("type")) {
        frame.type(H9Frame::from_underlying<H9Frame::Type>(res["type"].as<std::underlying_type_t<H9Frame::Type>>()));
    }

    if (res.count("flags")) {
        frame.flags(H9Frame::from_underlying<H9Frame::Flags>(res["flags"].as<std::uint8_t>()));
        raw = true;
    }

    if (res.count("dst_id")) {
        frame.destination_id(res["dst_id"].as<std::uint8_t>());
    }

    if (res.count("seqnum")) {
        frame.seqnum(res["seqnum"].as<std::uint8_t>());
        raw = true;
    }

    if (res.count("group")) {
        frame.broadcast_group(res["group"].as<std::uint16_t>());
    }

    frame.dlc(0);
    if (res.count("data")) {
        auto& v = res["data"].as<std::vector<std::string>>();
        std::vector<std::uint8_t> data;

        for (const auto& s : v) {
            data.push_back(std::stoul(s, nullptr, 16));
        }
        frame.dlc(data.size());
        frame.data(data);
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

    if (res.count("repeat")) {
        unsigned int sleep_time = res["repeat"].as<unsigned int>();

        while (true) {
            try {
                bus->send_frame(frame);
            }
            catch (std::system_error& e) {
                SPDLOG_ERROR("Can not send message: {}.", e.code().message());
                exit(EXIT_FAILURE);
            }
            catch (std::runtime_error& e) {
                SPDLOG_ERROR("Can not send message: {}.", e.what());
                exit(EXIT_FAILURE);
            }
            frame.seqnum(H9Frame::SEQNUM_MAX_VALUE + 1); //unvalid seqnum
            sleep(sleep_time);
        }
    }
    else {
        try {
            bus->send_frame(frame);
        }
        catch (std::system_error& e) {
            SPDLOG_ERROR("Can not send message: {}.", e.code().message());
            exit(EXIT_FAILURE);
        }
        catch (std::runtime_error& e) {
            SPDLOG_ERROR("Can not send message: {}.", e.what());
            exit(EXIT_FAILURE);
        }
    }
    return EXIT_SUCCESS;
}
