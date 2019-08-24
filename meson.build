project('yalr', 'cpp', version: '0.0.3-dev', default_options : ['cpp_std=c++17'])

# use subdir for this instead
# http://mesonbuild.com/Reference-manual.html#include_directories

libdir = include_directories('lib')

# subdir is just to get the names of files.
subdir('src')

yalr_exe = executable('yalr', yalr_srcfiles, include_directories: libdir)

clangtidy = find_program('clang-tidy', required: false)
if clangtidy.found()
    run_target(
        'tidy',
        command: [
            'scripts/run-clang-tidy.sh',
            clangtidy.path(),
            meson.build_root(),
            '-quiet',
            '-header-filter='+ meson.source_root() + '/.*'
        ] + yalr_srcfiles)
endif

# This order is important
subdir('examples')
subdir('test')
