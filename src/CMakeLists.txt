# yalr/src
cmake_minimum_required(VERSION 3.13)

##
## lib-include
##
add_library(lib-include INTERFACE)

target_include_directories(lib-include
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

##
## lib-src
##
## - Note: this is used by some of the tests to allow
## pulling in the actual cpp file as if it were a header
## to get to functions that are not announced in the
## true header (e.g. t05-parser)
##
add_library(lib-src INTERFACE)

target_include_directories(lib-src
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib")
target_link_libraries(lib-src
    INTERFACE lib-include
    )

##
## utils_objlib
##
add_library(utils_objlib OBJECT)

target_sources(utils_objlib
    PRIVATE
        "lib/utils.cpp"
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include/utils.hpp"
    )

target_link_libraries(utils_objlib 
    PUBLIC
        lib-include
    )

##
## sourcetext_objlib
##
add_library(sourcetext_objlib OBJECT)

target_sources(sourcetext_objlib
    PRIVATE
        "lib/sourcetext.cpp"
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include/sourcetext.hpp"
    )

target_link_libraries(sourcetext_objlib 
    PUBLIC
        lib-include
    )

##
## parsetree_objlib
##
add_library(parsetree_objlib OBJECT)

target_sources(parsetree_objlib
    PRIVATE
        "lib/parse_tree.cpp"
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include/parse_tree.hpp"
    )

target_link_libraries(parsetree_objlib
    PUBLIC
        lib-include
    )

##
## errorinfo_objlib
##
add_library(errorinfo_objlib OBJECT)

target_sources(errorinfo_objlib
    PRIVATE
        "lib/errorinfo.cpp"
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include/errorinfo.hpp"
    )

target_link_libraries(errorinfo_objlib 
    PUBLIC
        lib-include
        sourcetext_objlib
    )

##
## parser_objlib
##
add_library(parser_objlib OBJECT)

target_sources(parser_objlib
    PRIVATE
        "lib/parser.cpp"
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include/parser.hpp"
    )


target_link_libraries(parser_objlib 
    PUBLIC
        lib-include
        errorinfo_objlib
    )


##
## analyzer_objlib
##
add_library(analyzer_objlib OBJECT)

target_sources(analyzer_objlib
    PRIVATE
        "lib/analyzer.cpp"
        "include/analyzer_pass_1.hpp"
        "include/analyzer_pass_2.hpp"
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include/analyzer.hpp"
    )

target_link_libraries(analyzer_objlib 
    PUBLIC
        lib-include
        errorinfo_objlib
    )

##
## slr_objlib
##
add_library(slr_objlib OBJECT)

target_sources(slr_objlib
    PRIVATE
    "lib/algo/slr_tablegen.cpp"
    "lib/algo/slr_codegen.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/templates/slr_parser_template.hpp"
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/algo/slr.hpp"
    )

target_link_libraries(slr_objlib 
    PUBLIC
        lib-include
        inja
        nlohmann-json

    )
##
## codegen_objlib
##
add_library(codegen_objlib OBJECT)

target_sources(codegen_objlib
    PRIVATE
    "lib/codegen.cpp"
    "lib/codegen/nfa_machine.cpp"
    "lib/codegen/nfa_builder.cpp"
    "lib/codegen/dfa_builder.cpp"

    "${CMAKE_CURRENT_SOURCE_DIR}/include/templates/preamble.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/templates/main_func.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/templates/lexer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/templates/postlude.hpp"
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/codegen.hpp"
    )

target_link_libraries(codegen_objlib 
    PUBLIC
        lib-include
        inja
        nlohmann-json
        utils_objlib
    )

##
## translate_objlib
##
add_library(translate_objlib OBJECT)

target_sources(translate_objlib
    PRIVATE
    "lib/translate.cpp"
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/translate.hpp"
    )

target_link_libraries(translate_objlib 
    PUBLIC
        lib-include
        codegen_objlib
    )

