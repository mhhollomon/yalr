# yalr
cmake_minimum_required(VERSION 3.13)

project(Yalr VERSION 0.1.0
             DESCRIPTION "Yet another LR Parser Generator"
             LANGUAGES CXX)

#
# Make sure we use -std=c++17 or higher
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#
# Set the default build type to Release (if the user doesn't specify)
#
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
      "Debug" "Release" "Developer" "CI")
endif()


add_compile_options("$<$<CONFIG:Developer>:-g;-O1;-Wall;-pedantic>")
add_compile_options("$<$<CONFIG:CI>:-O1;-Wall;-pedantic>")
add_compile_options("$<$<CONFIG:Release>:-O3>")

add_subdirectory(extern)

add_subdirectory(src)

##
## yalr application
##
add_executable(yalr)

target_sources(yalr
    PRIVATE
        "src/main.cpp"
    )

target_link_libraries(yalr
    PRIVATE
        lib-include
        translate_objlib
        analyzer_objlib
        parser_objlib
        tablegen_objlib
        codegen_objlib
        errorinfo_objlib
        sourcetext_objlib
    )

#
# Uses the yalr executable
#
add_subdirectory(examples)

#
# Testing
#
enable_testing()
add_subdirectory(test)

#
# ctags
#
if(CMAKE_BUILD_TYPE STREQUAL "Developer")
    add_custom_target(ctags
        COMMAND ctags --options=${CMAKE_HOME_DIRECTORY}/.ctags -f ${CMAKE_BINARY_DIR}/tags .
        WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}"
        BYPRODUCTS tags
        VERBATIM
        COMMENT Generating ctags
        )
endif()
