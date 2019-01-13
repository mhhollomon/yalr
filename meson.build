project('yalr', 'cpp', default_options : ['cpp_std=c++17', 'b_pch=false'])

# use subdir for this instead
# http://mesonbuild.com/Reference-manual.html#include_directories

libdir = include_directories('lib')

# subdir is just to get the names of files.
subdir('src')

linkdep = dependency('boost', modules : ['system'])

executable('yalr', yalr_srcfiles, include_directories: libdir, dependencies : linkdep)

clangtidy = find_program('clang-tidy', required: false)
if clangtidy.found()
    run_target(
        'tidy',
        command: [
            '../scripts/run-clang-tidy.sh',
            clangtidy.path(),
            meson.build_root(),
            '-checks=*,-llvm-include-order,-cert-err58-cpp,-fuchsia-*,-google-runtime-references,-google-readability-todo',
            '-header-filter=\''+ meson.source_root() + '/*\'',
            '-warnings-as-errors=*',
        ] + yalr_srcfiles)
endif

subdir('test')
