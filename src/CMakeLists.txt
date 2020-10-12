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
add_library(lib-src INTERFACE)

target_include_directories(lib-src
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib")
target_link_libraries(lib-src
    INTERFACE lib-include
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
    "${CMAKE_CURRENT_SOURCE_DIR}/include/template.hpp"
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
    )
