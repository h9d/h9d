add_library(BusDriverLib STATIC
    bus_driver.cc bus_driver.h
    slcan_driver.cc slcan_driver.h
    loop_driver.cc loop_driver.h
    udp_driver.cc udp_driver.h
    ${H9_SOCKETCAN_DRIVER_SRCS}
    virtual_driver.cc virtual_driver.h
    pipe_driver.cc pipe_driver.h
    h9d_driver.cc h9d_driver.h)

target_include_directories(BusDriverLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(BusDriverLib
    PUBLIC CommonLib spdlog::spdlog nlohmann_json::nlohmann_json
    PRIVATE ${ATOMIC_LINK})
