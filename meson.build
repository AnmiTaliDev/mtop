project('mtop', 'c',
  version : '1.0.0',
  default_options : [
    'warning_level=3',
    'c_std=c17'
  ]
)

add_project_arguments('-D_POSIX_C_SOURCE=200809L', language : 'c')

inc_dirs = include_directories('src/Core', 'src/Config')

executable('mtop',
  sources : [
    'src/Core/main.c',
    'src/Core/system_info.c',
    'src/Core/display.c',
    'src/Core/screen.c',
    'src/Core/history.c',
    'src/Core/network.c',
    'src/Core/terminal.c',
    'src/Config/parser.c'
  ],
  include_directories : inc_dirs,
  install : true
)
