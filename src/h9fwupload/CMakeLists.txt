add_executable(h9fwupload
    h9fwupload.cc
    firmware_uploader.cc firmware_uploader.h)

target_include_directories(h9fwupload PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(h9fwupload PRIVATE ClientCommon nlohmann_json::nlohmann_json spdlog::spdlog)
install(TARGETS h9fwupload RUNTIME DESTINATION bin)
