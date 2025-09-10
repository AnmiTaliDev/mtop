# mtop

> Minimalistic Modern Top - A beautiful system monitor for Linux

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/license-Apache%202.0-green.svg)](LICENSE)
[![Meson](https://img.shields.io/badge/build-meson-orange.svg)](https://mesonbuild.com/)

## Shellshot

```
╭─────────────────────────────────────────────────────────────────────────────╮
│                              mtop - Modern Top                              │
╰─────────────────────────────────────────────────────────────────────────────╯
CPU: [███░░░░░░░░░░░░░░░░░░░░░░░░░░░░] 12.5%
MEM: [██████░░░░░░░░░░░░░░░░░░░░░░░░] 22.3% (3.4GB/15.0GB)
Load: 2.63 2.80 2.06  Processes: 350

┌─────────┬────────────────────┬─────────┬──────────────┬──────────────┐
│   PID   │        NAME        │  STATE  │     USER     │    MEMORY    │
├─────────┼────────────────────┼─────────┼──────────────┼──────────────┤
│    1842 │ firefox            │ S       │ user         │      536.2MB │
│    4807 │ electron           │ S       │ user         │      411.9MB │
│    1257 │ gnome-shell        │ S       │ user         │      271.8MB │
│    4282 │ vscodium           │ S       │ user         │      229.8MB │
└─────────┴────────────────────┴─────────┴──────────────┴──────────────┘
```

## Features

- 🎨 **Beautiful Unicode interface** with colors
- ⚡ **Fast and lightweight** - minimal system overhead
- 🔧 **Highly configurable** - config files + CLI options
- 📊 **Essential metrics** - CPU, Memory, Load, Processes
- 🎯 **Smart filtering** - hide/show processes by name or user
- 📋 **Multiple sorting** - by memory, CPU, PID, or name

## Quick Start

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install build-essential meson

# Clone and build
git clone https://github.com/AnmiTaliDev/mtop.git
cd mtop
meson setup build
meson compile -C build

# Run
./build/mtop
```

## Usage

```bash
# Basic usage
./mtop

# Custom options
./mtop --delay 5 --max-processes 15 --sort-cpu

# No colors (for scripts/old terminals)
./mtop --no-color

# Help
./mtop --help
```

## Configuration

Create `~/.config/mtop/config`:

```ini
[display]
update_interval = 2
max_processes = 20
show_colors = true

[processes]
sort_by = memory
hide_processes = kthreadd,ksoftirqd
show_kernel_threads = false
```

## Requirements

- Linux with /proc filesystem
- C++17 compiler (GCC 7+)
- Meson build system

## Project Structure

```
src/
├── Core/           # System monitoring logic
│   ├── main.cpp
│   ├── system_info.cpp
│   └── system_info.hpp
└── Config/         # Configuration management
    ├── parser.cpp
    └── parser.hpp
```

## License

Apache 2.0 License - see [LICENSE](LICENSE) file

## Contributing

Pull requests welcome! Please maintain the minimalistic design philosophy.

---

*Made with ❤️ for system administrators and developers*
