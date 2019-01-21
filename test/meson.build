t01_src = files('t01-skipparser.cpp')

t01_exe = executable( 't01-skipparser', t01_src, 
  include_directories: [src_inc, libdir],
  dependencies : linkdep
  )

test('t01-skipparser', t01_exe, suite: ['parser'])

##############################
t02_src = files('t02-quoted_pattern.cpp')

t02_exe = executable( 't02-quoted_pattern', t02_src, 
  include_directories: [src_inc, libdir],
  dependencies : linkdep
  )

test('t02-quoted_pattern', t02_exe, suite: ['parser'])

##############################
t03_src = files('t03-single-quote.cpp')

t03_exe = executable( 't03-single-quote', t03_src, 
  include_directories: [src_inc, libdir],
  dependencies : linkdep
  )

test('t03-single-quote', t03_exe, suite: ['parser'])

##############################
t10_src = files('t10-grammar.cpp') + parser_src

t10_exe = executable( 't10-grammar', t10_src, 
  include_directories: [src_inc, libdir],
  dependencies : linkdep
  )

test('t10-grammar', t10_exe, suite: ['parser'])

##############################
t20_src = files('t20-analyzer.cpp') + parser_src + analyzer_src

t20_exe = executable( 't20-analyzer', t20_src, 
  include_directories: [src_inc, libdir],
  dependencies : linkdep
  )

test('t20-analyzer', t20_exe, suite: ['analyzer'])

##############################

ta0_incfile = custom_target('ta0_inc', 
  output : ['ta0.hpp'],
  input  : yalr_grammar,
  command : [ yalr_exe, '-o', '@OUTPUT@', '@INPUT@' ]
  )

ta0_src = files('ta0-yalr.cpp') + [ ta0_incfile ]

ta0_exe = executable( 'ta0-yalr', ta0_src,
  include_directories: [src_inc, libdir],
  dependencies : [ linkdep ]
  )

test('ta0-yalr', ta0_exe, suite: ['yalr'])
