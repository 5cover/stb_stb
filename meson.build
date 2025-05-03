project('stb_stb', 'c',
  version: '0.1.0',
  default_options: ['warning_level=3']
)

add_project_arguments(
  '-DVERSION="@0@"'.format(meson.project_version()),
  language: 'c'
)

src_files = files(
  'src/stb_stb.c',
  'src/library_order.c',
  'src/lib/memlst.c',
  'src/lib/stb_ds.c',
)

executable('stb_stb', src_files)
