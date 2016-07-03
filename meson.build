project('MPSC FIFO', 'c')
add_global_arguments('-std=c11', '-O2', '-g', '-pthread', language : 'c')
glib_dep = dependency('glib-2.0')

inc = include_directories('include')

subdir('include')
subdir('src')
subdir('test')

pkgg = import('pkgconfig')

pkgg.generate(
  libraries : mpscfifolib,
  version : '0.1',
  name : 'libmpscfifo',
  filebase : 'mpscfifo',
  description : 'Multiple Producer Single Consumer FIFO implemented in C')

