# mtop

[![C](https://img.shields.io/badge/C-17-blue.svg)](https://en.cppreference.com/w/c/17)
[![License](https://img.shields.io/badge/license-GPL--3.0--only-green.svg)](LICENSE)
[![Meson](https://img.shields.io/badge/build-meson-orange.svg)](https://mesonbuild.com/)

## About

Terminal monitor for CPU, memory, network, and process usage on Linux.

## Dependencies

- Linux with the `/proc` filesystem
- C17 compiler (GCC or Clang)
- Meson and Ninja

## Build

```bash
git clone https://github.com/AnmiTaliDev/mtop.git
cd mtop
meson setup build
meson compile -C build
./build/mtop
```

## Usage

```bash
./build/mtop --delay 5 --max-processes 15 --sort-cpu
./build/mtop --no-color
./build/mtop --help
```

## Keys

| Key               | Action                    |
|-------------------|---------------------------|
| `q`, `Esc`        | Quit                      |
| `m`               | Sort by memory            |
| `c`               | Sort by CPU usage         |
| `p`               | Sort by PID               |
| `n`               | Sort by name              |
| `r`               | Reverse sort order        |
| `t`               | Toggle network statistics |
| `+`, `=`          | Decrease update interval  |
| `-`, `_`          | Increase update interval  |
| `h`, `?`          | Show help                 |

## Configuration

mtop reads `~/.config/mtop/config`. If that file does not exist, it reads
`/etc/mtop/config`. The `--config` option loads one more file on top of it.
Options are applied in the order they are given.

```ini
[display]
update_interval = 2
max_processes = 20
show_colors = true
show_load_avg = true
show_cpu_bar = true
show_memory_bar = true
show_network_stats = true
progress_bar_width = 30

[processes]
sort_by = memory
reverse_sort = false
show_process_state = true
show_process_user = true
show_kernel_threads = false
hide_processes = kworker,ksoftirqd
show_only_users =
```

| Key                  | Values                           |
|----------------------|----------------------------------|
| `update_interval`    | 1 to 60 seconds                  |
| `max_processes`      | 5 to 200                         |
| `progress_bar_width` | 10 to 100                        |
| `sort_by`            | `memory`, `cpu`, `pid`, `name`   |
| `hide_processes`     | Comma-separated parts of names   |
| `show_only_users`    | Comma-separated user names       |

## License

[GPL-3.0-only](LICENSE)
