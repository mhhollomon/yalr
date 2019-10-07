# yalr/examples
cmake_minimum_required(VERSION 3.13)

add_custom_command(
    OUTPUT calculator.yalr.hpp
    COMMAND yalr ${CMAKE_CURRENT_SOURCE_DIR}/calculator.yalr -o calculator.yalr.hpp
    DEPENDS yalr
    VERBATIM
    )

add_custom_target( gen_calc DEPENDS calculator.yalr.hpp )

add_executable(calculator)

target_sources(calculator
    PRIVATE
        "calculator_main.cpp"
    )

add_dependencies(calculator gen_calc)
target_include_directories(calculator
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
    )
