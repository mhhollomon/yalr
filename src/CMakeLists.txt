cmake_minimum_required(VERSION 3.6)
project(yalr LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

set (CLANG_TIDY_CHECKS "*")

find_program(
  CLANG_TIDY_EXE
  NAMES "clang-tidy"
  DOC "Path to clang-tidy executable"
  )

if (CLANG_TIDY_EXE)
	message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
	set(CLANG_TIDY_CHECKS "-*,modernize-*")
	set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=${CLANG_TIDY_CHECKS};-header-filter='${CMAKE_SOURCE_DIR}/*'"
	CACHE STRING "" FORCE)
else()
	message(AUTHOR_WARNING "clang-tidy not found!")
	set(CMAKE_CXX_CLANG_TIDY "" CACHE STRING "" FORCE) # delete it
endif()

include(cmake/clang-tidy.cmake)

add_compile_options("-g" "-O0" "-Wall" "-Wextra")

add_executable(yalr parser.cpp analyzer.cpp ast.cpp main.cpp)

#target_link_libraries(yalr boost_system)


