test1_src = files('test1.cpp') + parser_src

t1 = executable( 'test1', test1_src, 
  include_directories: [src_inc, libdir],
  dependencies : linkdep
  )

test('Test 1', t1, suite: ['parser'])
