add_library(ClientCommon STATIC
    h9connector.cc h9connector.h
    h9_configurator.cc h9_configurator.h)

target_include_directories(ClientCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ClientCommon PUBLIC CommonLib BusDriverLib)
