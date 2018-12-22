file(GLOB_RECURSE ALL_SOURCE_FILES *.cpp *.hpp)

#add_custom_target(
#        clang-format
#        COMMAND /usr/bin/clang-format
#        -style=file
#        -i
#        ${ALL_SOURCE_FILES}
#)

add_custom_target(
        clang-tidy
        COMMAND ${CMAKE_CXX_CLANG_TIDY}
        ${ALL_SOURCE_FILES}
        -config=''
        --
        -std=c++14
        ${INCLUDE_DIRECTORIES}
)
