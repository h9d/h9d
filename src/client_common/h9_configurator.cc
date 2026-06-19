/*
 * H9 project
 *
 * Created by crowx on 2023-09-16.
 *
 * Copyright (C) 2023 Kamil Palkowski. All rights reserved.
 */

#include "h9_configurator.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <pwd.h>
#include <unistd.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "version.h"
#include "libconfuse_helper.h"
#include "h9d_driver.h"
#include "loop_driver.h"
#include "pipe_driver.h"
#include "slcan_driver.h"
#include "udp_driver.h"
#ifdef H9_SOCKETCAN_DRIVER
#include "socketcan_driver.h"
#endif

namespace {
    void cfg_err_func(cfg_t* cfg, const char* fmt, va_list args) {
        /*std::string fmt_str = {fmt};
        fmt_str.erase(std::find_if(fmt_str.rbegin(), fmt_str.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), fmt_str.end());*/
        // Logger::default_log.vlog(Log::Level::INFO, __FILE__, __LINE__, fmt, args);
    }

    std::filesystem::path expand_tilde(const std::filesystem::path& p) {
        std::string s = p.string();

        if (s == "~" || s.rfind("~/", 0) == 0) {
            const char* home = std::getenv("HOME");
            if (!home) {
                struct passwd* pw = getpwuid(getuid());
                if (!pw) return p;
                home = pw->pw_dir;
            }
            s.replace(0, 1, home);
        }
        return std::filesystem::path(s);
    }

    std::vector<std::string> split_connection_string(const std::string& s) {
        std::vector<std::string> parts;
        std::size_t start = 0;
        std::size_t pos;
        while ((pos = s.find(':', start)) != std::string::npos) {
            parts.push_back(s.substr(start, pos - start));
            start = pos + 1;
        }
        parts.push_back(s.substr(start));
        return parts;
    }
} // namespace

H9Configurator::H9Configurator(const std::string& app_name, const std::string& app_desc):
    options(app_name, app_desc),
    _app_name(app_name),
    _app_desc(app_desc) {
    host = "";
    port = -1;
    source_id = default_source_id;
}

cxxopts::ParseResult H9Configurator::parse_command_line_arg(int argc, char** argv) {
    // clang-format off
    options.add_options("other")
#ifdef H9_DEBUG
            ("D,debug", "Enable debugging")
#endif
            ("h,help", "Show help")
            ("v,verbose", "Be more verbose")
            ("V,version", "Show version")
            ;
    options.add_options("connection")
            ("c,connect", "Connection URI\nh9d://<ip>[:<port>]\nslcan:///<tty path>\nsocketcan://<interface>\nudp://<src port>@<ip>:<port>", cxxopts::value<std::string>())
            ("F,config", "User config file", cxxopts::value<std::string>())
            ;
    // clang-format on

    add_app_specific_opt();

    try {
        cxxopts::ParseResult result = options.parse(argc, argv);

        debug = result.count("debug");

        if (result.count("help")) {
            std::cerr << options.help({"", "connection", "other"}) << std::endl;
            exit(EXIT_SUCCESS);
        }

        if (result.count("version")) {
            std::cerr << _app_name << " version " << APP_VERSION << " by crowx." << std::endl;
            std::cerr << "Copyright (C) 2017-2026 Kamil Palkowski. All rights reserved." << std::endl;
            exit(EXIT_SUCCESS);
        }

        verbose = result.count("verbose");

        if (result.count("connect")) {
            connection_uri = std::string(result["connect"].as<std::string>());
        }

        config_file = std::string(result["config"].as<std::string>());


        parse_app_specific_opt(result);

        return result;
    }
    catch (cxxopts::exceptions::parsing e) {
        std::cerr << e.what() << ". Try '-h' for more information." << std::endl;
        exit(EXIT_FAILURE);
    }
}

void H9Configurator::load_configuration() {
    cfg_opt_t cfg_connection_opts[] = {
        CFG_STR("uri", nullptr, CFGF_NONE | CFGF_NODEFAULT),
        CFG_INT("SourceID", 0, CFGF_NONE | CFGF_NODEFAULT),
        CFG_END()};
    cfg_opt_t cfg_opts[] = {
        CFG_INT("DefaultSourceID", default_source_id, CFGF_NONE),
        CFG_STR("default", nullptr, CFGF_NONE | CFGF_NODEFAULT),
        CFG_SEC("connection", cfg_connection_opts, CFGF_MULTI | CFGF_TITLE),
        CFG_END()};

    cfg = cfg_init(cfg_opts, CFGF_NONE);

    cfg_set_error_function(cfg, cfg_err_func);
    cfg_set_validate_func(cfg, "DefaultSourceID", confuse_helpers::validate_node_id);

    std::error_code ec;
    int result = CFG_SUCCESS;

    spdlog::level::level_enum cfg_read_erro_level = spdlog::level::warn;

    auto expand_default_user_config = expand_tilde(default_user_config);

    if (!config_file.empty()) {
        if (std::filesystem::exists(config_file, ec) && std::filesystem::is_regular_file(std::filesystem::status(config_file, ec))) {
            cfg_read_erro_level = spdlog::level::err;
            result = cfg_parse(cfg, config_file.c_str());
        }
        else {
            SPDLOG_ERROR("Cfg file '{}' doesn't exist.", config_file);
            cfg_free(cfg);
            cfg = nullptr;
        }
    }
    else if (std::filesystem::exists(expand_default_user_config, ec) && std::filesystem::is_regular_file(std::filesystem::status(expand_default_user_config, ec))) {
        result = cfg_parse(cfg, expand_default_user_config.c_str());
        config_file = expand_default_user_config;
    }
    else if (std::filesystem::exists(default_config, ec) && std::filesystem::is_regular_file(std::filesystem::status(default_config, ec))) {
        SPDLOG_INFO("Cfg file '{}' doesn't exist.", expand_default_user_config.c_str());
        result = cfg_parse(cfg, default_config);
        config_file = default_config;
    }
    else {
        SPDLOG_INFO("Cfg file '{}' doesn't exist.", expand_default_user_config.c_str());
        SPDLOG_INFO("Cfg file '{}' doesn't exist.", default_config);
        cfg_free(cfg);
        cfg = nullptr;
    }

    if (result == CFG_PARSE_ERROR) {
        SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), cfg_read_erro_level, "Can't parse cfg file: '{}'.", config_file);
        cfg_free(cfg);
        cfg = nullptr;
    }
    else if (result == CFG_FILE_ERROR) {
        SPDLOG_LOGGER_CALL(spdlog::default_logger_raw(), cfg_read_erro_level, "Can't parse cfg file: '{}'.", config_file);
        cfg_free(cfg);
        cfg = nullptr;
    }

    std::string tmp_uri_con = connection_uri;;

    if (!is_uri(connection_uri)) {
        connection_uri.clear();
    }

    if (cfg) {
        SPDLOG_INFO("Loading configuration from '{}' file.", config_file);

        source_id = cfg_getint(cfg, "DefaultSourceID");

        if (connection_uri.empty()) {
            if (tmp_uri_con.empty() && cfg_size(cfg, "default")) {
                tmp_uri_con = cfg_getstr(cfg, "default");
            }

            for (int i = 0; i < cfg_size(cfg, "connection"); i++) {
                cfg_t* h9d_sec = cfg_getnsec(cfg, "connection", i);
                if (tmp_uri_con == cfg_title(h9d_sec)) {
                    if (cfg_size(h9d_sec, "SourceID")) {
                        source_id = cfg_getint(h9d_sec, "SourceID");
                    }
                    if (cfg_size(h9d_sec, "uri")) {
                        connection_uri = cfg_getstr(h9d_sec, "uri");
                    }
                    break;
                }
            }
        }
    }

    if (connection_uri.empty()) {
        connection_uri = default_connection_uri;
    }
    try {
        parse_uri(connection_uri);
    }
    catch (const std::invalid_argument& e) {
        SPDLOG_ERROR("{}", e.what());
        exit(EXIT_FAILURE);
    }

    SPDLOG_DEBUG("uri scheme: {}, authority: {}, userinfo: {}, host: {}, port: {}, path: {}", scheme, authority, userinfo, host, port, path);
}

void H9Configurator::logger_initial_setup() {
    auto h9 = spdlog::stderr_color_st("stderr");
    spdlog::set_default_logger(h9);
}

void H9Configurator::logger_setup() {
    auto h9 = spdlog::default_logger();
    if (debug)
        h9->set_pattern(log_debug_pattern);
    else
        h9->set_pattern(log_pattern);

    int tmp_level = spdlog::level::err - verbose;
    if (tmp_level < spdlog::level::trace)
        h9->set_level(spdlog::level::trace);
    else
        h9->set_level(static_cast<spdlog::level::level_enum>(tmp_level));
}

bool H9Configurator::is_uri(const std::string& s) {
    return s.find("://") != std::string::npos;
}

void H9Configurator::parse_uri(const std::string& uri) {
    scheme.clear();
    authority.clear();
    userinfo = _app_name;
    host.clear();
    port = default_h9d_port;
    path.clear();

    auto colon = uri.find(':');
    if (colon == std::string::npos)
        throw std::invalid_argument("Invalid URI (missing scheme): " + uri);

    scheme = uri.substr(0, colon);
    std::string rest = uri.substr(colon + 1);

    if (rest.starts_with("//")) {
        rest = rest.substr(2);

        auto slash = rest.find('/');
        authority = (slash == std::string::npos) ? rest : rest.substr(0, slash);
        path      = (slash == std::string::npos) ? ""   : rest.substr(slash);

        std::string hostport;
        auto at = authority.find('@');
        if (at != std::string::npos) {
            userinfo = authority.substr(0, at);
            hostport = authority.substr(at + 1);
        } else {
            hostport = authority;
        }

        // rfind(':') handles IPv6 addresses like [::1]:port
        auto port_sep = hostport.rfind(':');
        if (port_sep != std::string::npos) {
            host = hostport.substr(0, port_sep);
            const std::string port_str = hostport.substr(port_sep + 1);
            if (!port_str.empty()) {
                try {
                    port = std::stoi(port_str);
                }
                catch (...) {
                    throw std::invalid_argument("Invalid port in URI: " + uri);
                }
            }
        } else {
            host = hostport;
        }
    } else {
        path = rest;
    }
}

std::unique_ptr<BusDriver> H9Configurator::get_bus_driver() {
    if (scheme == "slcan") {
        //if (parts.size() < 2)
        //    throw std::invalid_argument("slcan requires tty: slcan:///dev/ttyUSB0");
        return std::make_unique<SlcanDriver>(_app_name, path, "S4\rO\r");
    }
    if (scheme == "socketcan") {
#ifdef H9_SOCKETCAN_DRIVER
        if (parts.size() < 2)
            throw std::invalid_argument("socketcan requires interface: socketcan:can0");
        return std::make_unique<SocketCANDriver>(_app_name, authority);
#else
        throw std::invalid_argument("SocketCAN driver not available on this platform");
#endif
    }
    if (scheme == "udp") {
        //if (parts.size() < 4)
        //    throw std::invalid_argument("udp requires local_port:remote_addr:remote_port: udp:1211:127.0.0.1:1321");
        //return std::make_unique<UDPDriver>(_app_name, parts[1], parts[2], parts[3]);
    }
    if (scheme == "h9d") {
        //if (parts.size() < 3)
         //   throw std::invalid_argument("h9d requires hostname:port: h9d://127.0.0.1:1211");
        //return std::make_unique<H9DDriver>(_app_name, host, std::to_string(port), userinfo);
    }
    if (scheme == "pipe") {
        //if (parts.size() < 3)
        //    throw std::invalid_argument("pipe requires local_path:remote_path: pipe:/tmp/h9_a.sock:/tmp/h9_b.sock");
        //return std::make_unique<PipeDriver>(_app_name, parts[1], parts[2]);
    }
    if (scheme == "loop") {
        //return std::make_unique<LoopDriver>(_app_name);
    }

    throw std::invalid_argument("Unknown connection scheme '" + scheme + "'. "
        "Supported: slcan, socketcan, udp, h9, pipe, loop");
}

H9Connector H9Configurator::get_connector() {
    return H9Connector(host, port > 0 ? std::to_string(port) : "");
}

std::uint16_t H9Configurator::get_default_source_id() {
    return source_id;
}

std::string H9Configurator::get_host() const {
    return host;
}

std::string H9Configurator::get_port() const {
    return port > 0 ? std::to_string(port) : "";
}

bool H9Configurator::get_debug() const {
    return debug;
}
