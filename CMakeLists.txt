# yalr
cmake_minimum_required(VERSION 3.13)

project(Yalr VERSION 0.2.0
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

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(YALR_DEFAULT_INSTALL_OPTION ON)
else()
    set(YALR _DEFAULT_INSTALL_OPTION OFF)
endif()

option(YALR_BUILD_EXAMPLES "Set to ON to build examples" ON)
option(YALR_BUILD_TESTS "Set to ON to build tests" ON)
option(YALR_ENABLE_INSTALL "Generate the install target" ${YALR_DEFAULT_INSTALL_OPTION})


add_compile_options("$<$<CONFIG:Developer>:-g;-O0;-Wall;-pedantic>")
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
        parsetree_objlib
        parser_objlib
        tablegen_objlib
        codegen_objlib
        errorinfo_objlib
        sourcetext_objlib
    )

#
# Uses the yalr executable
#
if (YALR_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

#
# Testing
#
if (YALR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

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


#
# install
#
if (YALR_ENABLE_INSTALL)
    install(TARGETS yalr DESTINATION bin)
endif()
