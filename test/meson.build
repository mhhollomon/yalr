t01_src = files('t01-skipparser.cpp') + parser_src

t01_exe = executable( 't01-skipparser', t01_src, 
  include_directories: [src_inc, libdir],
  dependencies : linkdep
  )

test('t01-skipparser', t01_exe, suite: ['parser'])

##############################
t02_src = files('t02-quoted_pattern.cpp') + parser_src

t02_exe = executable( 't02-quoted_pattern', t02_src, 
  include_directories: [src_inc, libdir],
  dependencies : linkdep
  )

test('t02-quoted_pattern', t02_exe, suite: ['parser'])
