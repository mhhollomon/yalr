t05_src = files('t05-parser.cpp')

t05_exe = executable( 't05-parser', t05_src, 
  include_directories: [src_inc, libdir],
  )

test('t05-parser', t05_exe, suite: ['parser'])

##############################
t10_src = files('t10-grammar.cpp')

t10_exe = executable( 't10-grammar', t10_src, 
  include_directories: [src_inc, libdir],
  )

test('t10-grammar', t10_exe, suite: ['parser'])

##############################
t20_src = files('t20-analyzer.cpp') + analyzer_src

t20_exe = executable( 't20-analyzer', t20_src, 
  include_directories: [src_inc, libdir],
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
  )

test('ta0-yalr', ta0_exe, suite: ['yalr'])
