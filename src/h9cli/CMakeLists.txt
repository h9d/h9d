BISON_TARGET(cli_parser cli_parser.y ${CMAKE_CURRENT_BINARY_DIR}/cli_parser.cc
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/cli_parser.h)
FLEX_TARGET(cli_scaner cli_lexer.l ${CMAKE_CURRENT_BINARY_DIR}/cli_lexer.cc)
ADD_FLEX_BISON_DEPENDENCY(cli_scaner cli_parser)

add_executable(h9cli
    h9cli.cc
    cli_parsing_driver.cc cli_parsing_driver.h
    cli_cache.cc cli_cache.h
    command_executor.cc command_executor.h
    ${BISON_cli_parser_OUTPUTS}
    ${FLEX_cli_scaner_OUTPUTS})

target_include_directories(h9cli PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(h9cli PRIVATE
    ClientCommon nlohmann_json::nlohmann_json spdlog::spdlog ${READLINE_LIBRARIES})

install(TARGETS h9cli RUNTIME DESTINATION bin)
