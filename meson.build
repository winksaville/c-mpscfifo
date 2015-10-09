project('Multiple Producer Single Consumer FIFO in C', 'c')
add_global_arguments('-std=c11', language : 'c')
glib_dep = dependency('glib-2.0')

inc = include_directories('include')

subdir('include')
subdir('src')
subdir('test')

pkgconfig_gen(
  libraries : mpscfifolib,
  version : '0.1',
  name : 'libmpscfifo',
  filebase : 'mpscfifo',
  description : 'A Multiple Producer Single Consumer Fifo')

