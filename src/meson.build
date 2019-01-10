project('yalr', 'cpp', default_options : ['cpp_std=c++17'])

# use subdir for this instead
# http://mesonbuild.com/Reference-manual.html#include_directories
add_project_arguments('-I../lib', language: ['cpp'])

srcfiles = files('codegen.cpp','SymbolTable.cpp', 'tablegen.cpp','parser.cpp','analyzer.cpp','ast.cpp','main.cpp')
executable('yalr', srcfiles)

clangtidy = find_program('clang-tidy', required: false)
if clangtidy.found()
    run_target(
        'tidy',
        command: [
            '../scripts/run-clang-tidy.sh',
            clangtidy.path(),
            meson.build_root(),
            '-checks=*,-llvm-include-order,-cert-err58-cpp,-fuchsia-*,-google-runtime-references',
            '-header-filter=\''+ meson.source_root() + '/*\'',
            '-warnings-as-errors=*',
        ] + srcfiles)
endif
