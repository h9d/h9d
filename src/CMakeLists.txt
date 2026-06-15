if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    message("-! Using epoll")
    set(H9D_EVENT_SRCS epoll.cc epoll.h)
    message("-! Enable SocketCAN driver")
    set(H9_SOCKETCAN_DRIVER_SRCS socketcan_driver.cc socketcan_driver.h)
    set(H9_SOCKETCAN_DRIVER TRUE)
else()
    message("-! Using kqueue")
    set(H9D_EVENT_SRCS kqueue.cc kqueue.h)
    message("-! Disable SocketCAN driver")
    set(H9_SOCKETCAN_DRIVER_SRCS "")
    set(H9_SOCKETCAN_DRIVER "")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-psabi)
    set(ATOMIC_LINK atomic)
endif()

if(H9_DEBUG)
    message("-! Debug enable")
    set(H9_DEBUG TRUE)
    set(NDEBUG "")
else()
    message("-! Debug disable")
    set(H9_DEBUG "")
    set(NDEBUG TRUE)
endif()

configure_file(common/config_gen.h.in config_gen.h)

add_compile_definitions(SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

set(H9_SRC_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/git_version.h
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/generate_git_version_header.py > ${CMAKE_CURRENT_BINARY_DIR}/git_version.h~
    COMMAND /bin/sh -c 'diff ${CMAKE_CURRENT_BINARY_DIR}/git_version.h~ ${CMAKE_CURRENT_BINARY_DIR}/git_version.h 2> /dev/null > /dev/null || { cp ${CMAKE_CURRENT_BINARY_DIR}/git_version.h~ ${CMAKE_CURRENT_BINARY_DIR}/git_version.h\; }')

add_custom_target(GitVersionHeader DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/git_version.h)

add_subdirectory(common)
add_subdirectory(bus_driver)
add_subdirectory(client_common)
add_subdirectory(devs)
add_subdirectory(h9d)
add_subdirectory(h9sniffer)
add_subdirectory(h9send)
add_subdirectory(h9stats)
add_subdirectory(h9fwupload)
add_subdirectory(h9cli)
