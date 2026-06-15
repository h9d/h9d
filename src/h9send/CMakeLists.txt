add_executable(h9send h9send.cc)
target_link_libraries(h9send PRIVATE ClientCommon nlohmann_json::nlohmann_json spdlog::spdlog)
install(TARGETS h9send RUNTIME DESTINATION bin)
