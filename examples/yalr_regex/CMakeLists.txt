# yalr/examples/regex
cmake_minimum_required(VERSION 3.13)

add_custom_command(
    OUTPUT regex.hpp
    COMMAND yalr ${CMAKE_CURRENT_SOURCE_DIR}/regex.yalr -o regex.hpp
    DEPENDS yalr regex.yalr
    VERBATIM
    )

add_custom_target( gen_regex DEPENDS regex.yalr regex.hpp)

add_executable(greppy)

target_include_directories(greppy
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
    )

target_link_libraries(greppy
    PRIVATE
        cxxopts
    )

target_sources(greppy
    PRIVATE
        "main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/rpn.hpp"
        "${CMAKE_CURRENT_BINARY_DIR}/regex.hpp"
        "rpn.cpp"
    )

add_dependencies(greppy gen_regex)
