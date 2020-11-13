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

add_executable(t11-termset)
target_sources(t11-termset PRIVATE "t11-termset.cpp")
target_link_libraries(t11-termset
    PRIVATE doctest lib-include
        parser_objlib
        errorinfo_objlib
        sourcetext_objlib)

add_test(NAME t11-termset COMMAND "t11-termset")


## analyzer #############################
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

add_executable(t21-termset-analyze)
target_sources(t21-termset-analyze PRIVATE "t21-termset-analyze.cpp")
target_link_libraries(t21-termset-analyze
    PRIVATE doctest lib-include
        parser_objlib
        analyzer_objlib
        sourcetext_objlib
        errorinfo_objlib
    )
add_test(NAME t21-termset-analyze COMMAND "t21-termset-analyze")

add_executable(t60-nfa_builder)
target_sources(t60-nfa_builder PRIVATE "t60-nfa_builder.cpp")
target_link_libraries(t60-nfa_builder
    PRIVATE doctest lib-include
        utils_objlib
        codegen_objlib
        analyzer_objlib
        sourcetext_objlib
    )
add_test(NAME t60-nfa_builder COMMAND "t60-nfa_builder")

add_executable(t61-dfa_builder)
target_sources(t61-dfa_builder PRIVATE "t61-dfa_builder.cpp")
target_link_libraries(t61-dfa_builder
    PRIVATE doctest lib-include
        utils_objlib
        codegen_objlib
        analyzer_objlib
        sourcetext_objlib
    )
add_test(NAME t61-dfa_builder COMMAND "t61-dfa_builder")

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

add_executable(tff-regex)
target_sources(tff-regex PRIVATE "tff-regex.cpp")
target_link_libraries(tff-regex
    PRIVATE doctest lib-include
        utils_objlib
    )
add_test(NAME tff-regex COMMAND "tff-regex")
