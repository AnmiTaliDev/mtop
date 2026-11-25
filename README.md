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
./build/mtop --delay 5 --sort-cpu
./build/mtop --no-color
./build/mtop --help
```

The terminal must be at least 60 columns wide and 16 rows high. The network
panel shows the interface of the default IPv4 route. If there is no such route,
it shows the interface with the most traffic. With `--no-color` or
`show_colors = false`, the interface is drawn with ASCII characters and no
colors. It still uses terminal control sequences.

## Keys

| Key                 | Action                                |
|---------------------|---------------------------------------|
| `q`                 | Quit                                  |
| `Up`, `Down`        | Move the selection                    |
| `PgUp`, `PgDn`      | Move one page                         |
| `Home`, `End`       | Select the first or last process      |
| `/`                 | Find by name, ignoring case           |
| `Enter`             | Keep the search and leave input       |
| `Esc`               | Clear the search, or quit             |
| `k`                 | Send `SIGTERM` after confirming `y`   |
| `m`, `c`, `p`, `n`  | Sort by memory, CPU, PID, or name     |
| `r`                 | Reverse the sort order                |
| `t`                 | Toggle the network panel              |
| `+`, `=`            | Decrease the update interval          |
| `-`, `_`            | Increase the update interval          |
| `h`, `?`            | Show help                             |

Until the selection is moved, it stays on the first row. After that it follows
the selected process. `Home` returns it to the first row.

## Configuration

mtop reads `~/.config/mtop/config`. If that file does not exist, it reads
`/etc/mtop/config`. The `--config` option loads one more file on top of it.
Options are applied in the order they are given.

```ini
[display]
update_interval = 2
show_colors = true
theme = default
show_load_avg = true
show_cpu_bar = true
show_memory_bar = true
show_network_stats = true

[processes]
sort_by = memory
reverse_sort = false
show_process_state = true
show_process_user = true
show_kernel_threads = false
hide_processes = kworker,ksoftirqd
show_only_users =
```

| Key               | Values                              |
|-------------------|-------------------------------------|
| `update_interval` | 1 to 60 seconds                     |
| `theme`           | `default`, `ocean`, `amber`, `mono` |
| `sort_by`         | `memory`, `cpu`, `pid`, `name`      |
| `hide_processes`  | Comma-separated parts of names      |
| `show_only_users` | Comma-separated user names          |

The `show_cpu_bar` and `show_memory_bar` keys control the CPU graph and the
memory bars. The `max_processes` and `progress_bar_width` keys and the
`--max-processes` option are accepted and ignored.

## License

[GPL-3.0-only](LICENSE)
