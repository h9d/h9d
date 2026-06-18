add_executable(h9d
    h9d.cc
    h9d_configurator.cc h9d_configurator.h
    ${H9D_EVENT_SRCS}
    busframe.cc busframe.h
    metrics_collector.cc metrics_collector.h
    send_frame_result.h
    bus_endpoint.cc bus_endpoint.h
    bus.cc bus.h
    framesubject.cc framesubject.h
    frameobserver.cc frameobserver.h
    h9framecomparator.cc h9framecomparator.h
    tcpserver.cc tcpserver.h
    tcpclientthread.cc tcpclientthread.h
    api.cc api.h
    client_frame_obs.cc client_frame_obs.h
    raw_node.cc raw_node.h
    node_mgr.cc node_mgr.h
    node.cc node.h
    node_desc_loader.cc node_desc_loader.h
    virtual_endpoint.cc virtual_endpoint.h
    virtual_py_node.cc virtual_py_node.h
    py_h9.cc py_h9.h
    dev_node_exception.cc dev_node_exception.h
    dev.cc dev.h
    dev_status_observer.cc dev_status_observer.h
    dev_loader.cc dev_loader.h
    dev_workers.cc dev_workers.h)

target_sources(h9d PRIVATE $<TARGET_OBJECTS:DevsLib>)

target_include_directories(h9d PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Python3_INCLUDE_DIRS})

target_link_libraries(h9d PRIVATE
    BusDriverLib CommonLib ${ATOMIC_LINK} nlohmann_json::nlohmann_json spdlog::spdlog Python3::Python)

install(TARGETS h9d RUNTIME DESTINATION bin)
