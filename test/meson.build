testexe = executable(
  'testmpscfifo',
  ['test.c', 'multi_thread.c'],
  include_directories : inc,
  link_args : ['-Wl,-lpthread'],
  link_with : mpscfifolib)

test('lib mpscfifo test', testexe)
