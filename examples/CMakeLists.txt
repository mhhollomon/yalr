# yalr/examples
cmake_minimum_required(VERSION 3.13)

add_custom_command(
    OUTPUT calculator.cpp
    COMMAND yalr ${CMAKE_CURRENT_SOURCE_DIR}/calculator.yalr -o calculator.cpp
    DEPENDS yalr calculator.yalr
    VERBATIM
    )

add_custom_target( gen_calc DEPENDS calculator.cpp calculator.yalr)

add_executable(calculator)

target_sources(calculator
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/calculator.cpp"
    )

add_dependencies(calculator gen_calc)

add_subdirectory(replcalc)
