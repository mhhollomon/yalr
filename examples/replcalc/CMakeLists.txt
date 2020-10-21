# yalr/examples/replcalc
cmake_minimum_required(VERSION 3.13)

add_custom_command(
    OUTPUT parser.hpp
    COMMAND yalr ${CMAKE_CURRENT_SOURCE_DIR}/calculator.yalr -o parser.hpp
    DEPENDS yalr calculator.yalr
    VERBATIM
    )

add_custom_target( gen_repl_parser DEPENDS parser.hpp calculator.yalr)

add_executable(replcalc)

target_include_directories(replcalc
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
    )

target_link_libraries(replcalc
    PRIVATE
        cxxopts
    )

target_sources(replcalc
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
        "${CMAKE_CURRENT_BINARY_DIR}/parser.hpp"
    )

add_dependencies(replcalc gen_repl_parser)
