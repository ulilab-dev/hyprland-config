# hyprbar

A minimal, modern status bar for [Hyprland](https://hyprland.org), written in C++.

```
┌───────────────────────────────────────────────────────────────────────────────────────────────┐
│  [1] [2] [3]      My Terminal — nvim            CPU ██░░ 23%  RAM ████ 61%  Wed 28 May  14:32 │
└───────────────────────────────────────────────────────────────────────────────────────────────┘
```

## Features

| Section | Contents |
|---------|----------|
| **Left** | Workspaces (active = accent pill, occupied = outline, empty = dim) |
| **Centre** | Active window title (auto-truncated) |
| **Right** | CPU % bar · RAM % bar · Clock |

- Transparent ARGB background with rounded corners
- Live updates via Hyprland IPC event socket
- Single binary, no runtime deps beyond system libraries

## Dependencies

| Package | Arch | Ubuntu/Debian |
|---------|------|---------------|
| `wayland-client` | `wayland` | `libwayland-dev` |
| `cairo` | `cairo` | `libcairo2-dev` |
| `wayland-scanner` | `wayland` | `wayland-scanner` |
| `cmake ≥ 3.20` | `cmake` | `cmake` |
| `g++ / clang++ (C++20)` | `gcc` / `clang` | `g++` |
| A Nerd Font or JetBrains Mono | (optional) | (optional) |

### Arch Linux
```bash
sudo pacman -S wayland cairo cmake gcc
```

### Ubuntu 24.04+
```bash
sudo apt install libwayland-dev libcairo2-dev wayland-scanner cmake g++
```

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Binary: `build/hyprbar`

## Install

```bash
sudo cmake --install build   # installs to /usr/local/bin/hyprbar
```

Or just copy `build/hyprbar` anywhere on your `$PATH`.

## Run

```bash
hyprbar &
```

Add to your `~/.config/hypr/hyprland.lua`:

```ini
hl.exec_cmd("hyprbar")
```

## Configuration

Edit the `BarConfig` struct in `src/Bar.hpp` and rebuild:

```cpp
struct BarConfig {
    int    height     = 32;     // bar height in pixels
    int    margin_top = 6;      // gap from top edge of screen
    double bg_r       = 0.078;  // background R
    double bg_g       = 0.082;  // background G
    double bg_b       = 0.122;  // background B
    double bg_a       = 0.92;   // background opacity
    double corner_r   = 10.0;   // corner radius
};
```

Colour reference (defaults → dark navy, close to Catppuccin Mocha base):
- Background: `#14151F` @ 92% opacity
- Accent:     `#7FC2F6` (blue)
- Active workspace pill: accent fill, dark text
- Occupied workspace: accent outline + faint fill
- Empty workspace: dim ghost pill

## Architecture

```
Bar            — Wayland surface, SHM buffer, Cairo rendering, main loop
HyprIPC        — Unix-socket IPC with Hyprland (.socket / .socket2)
modules/
  Workspaces   — Workspace pills, subscribes to IPC change events
  WindowTitle  — Active window title, updates on activewindow events
  Clock        — Date + time, ticks every second in main loop
  SysInfo      — CPU% + RAM% bars, reads /proc/stat & /proc/meminfo
```

## Troubleshooting

**`No zwlr_layer_shell_v1`**  
Your compositor doesn't advertise `wlr-layer-shell`. Hyprland supports it; make sure you're actually running Hyprland.

**`HYPRLAND_INSTANCE_SIGNATURE not set`**  
The bar must be launched inside a Hyprland session (or with the env var set manually for testing).

**Workspaces don't update**  
Check that `$XDG_RUNTIME_DIR/hypr/<sig>/.socket2.sock` exists and is accessible.

**Font looks wrong**  
Install `ttf-jetbrains-mono` (Arch) / `fonts-jetbrains-mono` (Ubuntu) or change the font name in the module `.draw()` calls.

## License

MIT
