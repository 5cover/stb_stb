project('stb_stb', 'c',
  version: '0.1.0',
  default_options: ['warning_level=3', 'c_std=c11']
)

add_project_arguments(
  '-DVERSION="@0@"'.format(meson.project_version()),
  language: 'c'
)

src_dir = 'src'
lib_dir = 'src/lib'

src_files = files(
  'src/stb_stb.c',
  'src/library_order.c',
  'src/lib/memlst.c',
  'src/lib/stb_ds.c',
)

inc_dirs = include_directories(src_dir, lib_dir)

executable('stb_stb', src_files, include_directories: inc_dirs)
