add_library(CommonLib STATIC
    h9_frame.cc h9_frame.h
    h9socket.cc h9socket.h
    h9msgsocket.cc h9msgsocket.h
    fmt_helpers.cc fmt_helpers.h
    libconfuse_helper.cc libconfuse_helper.h)

target_include_directories(CommonLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CONFUSE_INCLUDE_DIRS})

target_compile_options(CommonLib PUBLIC ${CONFUSE_CFLAGS_OTHER})

target_link_libraries(CommonLib PUBLIC fmt::fmt ${CONFUSE_LIBRARIES})

# ---------------------------------------------------------------------------
# version.h generation
# ---------------------------------------------------------------------------
set(VERSION_H_IN  "${CMAKE_SOURCE_DIR}/src/common/version.h.in")
set(VERSION_H_OUT "${CMAKE_BINARY_DIR}/src/version.h")
set(VERSION_DEPENDS_TARGET CommonLib)

include(${H9CAN_PATH}/cmake/git-version.cmake)
