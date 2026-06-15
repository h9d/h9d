add_executable(h9sniffer h9sniffer.cc)
target_link_libraries(h9sniffer PRIVATE ClientCommon nlohmann_json::nlohmann_json spdlog::spdlog)
install(TARGETS h9sniffer RUNTIME DESTINATION bin)
