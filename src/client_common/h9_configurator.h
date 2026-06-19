/*
* H9 project
*
* Created by crowx on 2023-09-16.
*
* Copyright (C) 2023 Kamil Palkowski. All rights reserved.
*/

#pragma once

#include "h9connector.h"
#include <confuse.h>
#include <cxxopts/cxxopts.hpp>
#include <map>
#include <memory>

#include "bus_driver.h"

class H9Configurator {
  private:
    constexpr static char log_debug_pattern[] = "%^[%L %T.%e] [%s:%#]%$ %v";
    //constexpr static char log_pattern[] = "%^[%L]%$ %v";
    constexpr static char log_pattern[] = "%v";

    constexpr static const char* default_config = H9_CONFIG_FILE;
    constexpr static const char* default_user_config = H9_USER_CONFIG_FILE;
    constexpr static char default_connection_uri[] = "h9d://localhost";
    constexpr static int default_h9d_port = H9D_DEFAULT_PORT;
    constexpr static int default_source_id = DEFAULT_SOURCE_ID_FOR_CLIENT;
  protected:
    cfg_t* cfg;
    cxxopts::Options options;

    const std::string _app_name;
    const std::string _app_desc;

    bool debug;
    int verbose;
    std::string connection_uri;

    std::string scheme;     // (h9d, slcan, socketcan, udp, pipe, loop)
    std::string authority;  // (192.168.1.10:10001, can0) == userinfo@host:port
    std::string userinfo;
    std::string host;
    int port;
    std::string path;       // (/dev/ttyUSB0)
    std::map<std::string, std::string> query;

    std::string config_file;
    int source_id;

    static bool is_uri(const std::string&);
    void parse_uri(const std::string& uri);

    virtual void add_app_specific_opt() {}
    virtual void parse_app_specific_opt(const cxxopts::ParseResult& result) {}
  public:
    H9Configurator(const std::string& app_name, const std::string& app_desc);
    cxxopts::ParseResult parse_command_line_arg(int argc, char** argv);
    void load_configuration();
    void logger_initial_setup();
    void logger_setup();

    H9Connector get_connector();
    std::unique_ptr<BusDriver> get_bus_driver();
    std::uint16_t get_default_source_id();

    std::string get_host() const;
    std::string get_port() const;
    bool get_debug() const;
};
