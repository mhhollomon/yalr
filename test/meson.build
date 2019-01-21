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

##############################

ta0_incfile = custom_target('ta0_inc', 
  output : ['ta0.hpp'],
  input  : yalr_grammar,
  command : [ yalr_exe, '-o', '@OUTPUT@', '@INPUT@' ]
  )

ta0_src = files('ta0-yalr.cpp')

ta0_exe = executable( 'ta0-yalr', [ ta0_src, ta0_incfile], 
  include_directories: [src_inc, libdir],
  dependencies : [ linkdep ]
  )

test('ta0-yalr', ta0_exe, suite: ['yalr'])
