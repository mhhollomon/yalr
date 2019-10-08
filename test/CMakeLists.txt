# yalr/test
cmake_minimum_required(VERSION 3.13)

set(YALR_RUNNER_FLAGS -std=c++17)

add_executable(test_runner)
target_sources(test_runner PRIVATE "runner.cpp")
target_link_libraries(test_runner
    PRIVATE doctest)
target_link_options(test_runner PRIVATE "-lstdc++fs")


## utils ##################################
add_executable(t01-utils)
target_sources(t01-utils PRIVATE "t01-utils.cpp")
target_link_libraries(t01-utils
    PRIVATE doctest lib-include)

add_test(NAME t01-utils COMMAND "t01-utils")

## sourcetext #############################
add_executable(t02-sourcetext)
target_sources(t02-sourcetext PRIVATE "t02-sourcetext.cpp")
target_link_libraries(t02-sourcetext
    PRIVATE doctest 
        lib-include
        sourcetext_objlib
    )

add_test(NAME t02-sourcetext COMMAND "t02-sourcetext")

add_test(NAME t02-sourcetext-1 COMMAND "test_runner"
    "${CMAKE_CURRENT_SOURCE_DIR}/runner_configs/t02.1.cfgfile"
    "include='-I$<JOIN:$<TARGET_PROPERTY:yalr,INCLUDE_DIRECTORIES>, -I>'"
    "flags=${YALR_RUNNER_FLAGS}"
    "compiler=${CMAKE_CXX_COMPILER}"
    )

## parser #############################
add_executable(t05-parser)
target_sources(t05-parser PRIVATE "t05-parser.cpp")
target_link_libraries(t05-parser
    PRIVATE doctest lib-src errorinfo_objlib
        sourcetext_objlib
    )

add_test(NAME t05-parser COMMAND "t05-parser")

#add_executable(t08-symboltable)
#target_sources(t08-symboltable PRIVATE "t08-symboltable.cpp")
#target_link_libraries(t08-symboltable
#    PRIVATE doctest lib-include symboltable_objlib)

add_executable(t10-grammar)
target_sources(t10-grammar PRIVATE "t10-grammar.cpp")
target_link_libraries(t10-grammar
    PRIVATE doctest lib-include
        parser_objlib
        errorinfo_objlib
        sourcetext_objlib)

add_test(NAME t10-grammar COMMAND "t10-grammar")

add_executable(t20-analyzer)
target_sources(t20-analyzer PRIVATE "t20-analyzer.cpp")
target_link_libraries(t20-analyzer
    PRIVATE doctest lib-include
        parser_objlib
        analyzer_objlib
        sourcetext_objlib
        errorinfo_objlib
    )
add_test(NAME t20-analyzer COMMAND "t20-analyzer")

add_test(NAME t30-yalr-1 COMMAND "test_runner"
    "${CMAKE_CURRENT_SOURCE_DIR}/runner_configs/t30.1.cfgfile"
    "yalr='$<TARGET_FILE:yalr>'"
    )

add_test(NAME t30-yalr-2 COMMAND "test_runner"
    "${CMAKE_CURRENT_SOURCE_DIR}/runner_configs/t30.2.cfgfile"
    "yalr='$<TARGET_FILE:yalr>'"
    )

add_test(NAME t30-yalr-3 COMMAND "test_runner"
    "${CMAKE_CURRENT_SOURCE_DIR}/runner_configs/t30.3.cfgfile"
    "yalr='$<TARGET_FILE:yalr>'"
    )
