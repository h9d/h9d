add_executable(h9stats h9stats.cc)
target_link_libraries(h9stats PRIVATE ClientCommon nlohmann_json::nlohmann_json spdlog::spdlog)
install(TARGETS h9stats RUNTIME DESTINATION bin)
