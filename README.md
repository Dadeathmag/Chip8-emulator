# CHIP-8 Emulator

A CHIP-8 emulator written in C++20 with an SDL2 front end. It runs `.ch8` ROMs at a configurable CPU speed, with a built-in toolbar for loading games, changing display scale and color palette, and adjusting emulation settings.

## Media

<img width="650" height="394" alt="chip8" src="https://github.com/user-attachments/assets/f84186ed-cdf7-47a7-9716-430d7530eb5e" />

Pong.ch8

<img width="778" height="458" alt="VID-20260624-WA0188" src="https://github.com/user-attachments/assets/15ad14b5-bbd5-4d12-ad45-3cd6271bf976" />

Quirk-test by Timendus

<img width="606" height="360" alt="VID-20260624-WA0189" src="https://github.com/user-attachments/assets/040a3113-0576-4c30-a306-cf9ee51d34a2" />

beep.ch8

<img width="612" height="360" alt="VID-20260624-WA0190" src="https://github.com/user-attachments/assets/8fea2f98-7ce2-48a1-bd87-06a54b23bbfe" />

random.ch8

<img width="616" height="360" alt="VID-20260624-WA0187" src="https://github.com/user-attachments/assets/5764e82c-6542-4b53-9e87-96f2d1b5f3e2" />

tetris.ch8

## Requirements

- **CMake** 3.16 or newer
- **C++20** compiler (GCC, Clang, or MSVC)
- **SDL2** development libraries

### Install SDL2 (Linux / WSL)

```bash
# Debian / Ubuntu
sudo apt update
sudo apt install build-essential cmake libsdl2-dev

# Fedora
sudo dnf install cmake gcc-c++ SDL2-devel

# Arch
sudo pacman -S cmake gcc sdl2
```

## Build

From the project root:

```bash
cmake -B build
cmake --build build
```

Release build (optional):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Clean rebuild:

```bash
rm -rf build
cmake -B build
cmake --build build
```

The executable is written to `build/chip8`.

## Run

Place `.ch8` ROM files in a `roms/` directory next to the executable (or run from the project root so `roms/` resolves correctly):

```bash
./build/chip8
```

On startup, the emulator loads the first `.ch8` file found in `roms/` (sorted alphabetically). If no ROMs are present, the display shows **NO ROM LOADED** until you pick one from the toolbar.

## Controls

CHIP-8 uses a 4×4 keypad. This emulator maps it to the keyboard as follows:

```
┌───┬───┬───┬───┐      ┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ 4 │      │ 1 │ 2 │ 3 │ C │     
├───┼───┼───┼───┤      ├───┼───┼───┼───┤ 
│ Q │ W │ E │ R │      │ 4 │ 5 │ 6 │ D │     
├───┼───┼───┼───┤ ===> ├───┼───┼───┼───┤     
│ A │ S │ D │ F │      │ 7 │ 8 │ 9 │ E │
├───┼───┼───┼───┤      ├───┼───┼───┼───┤
│ Z │ X │ C │ V │      │ A │ 0 │ B │ F │
└───┴───┴───┴───┘      └───┴───┴───┴───┘
```

Close the window or use the window manager quit shortcut to exit.

## Toolbar

The top bar provides emulator controls without leaving the window:

| Control | Action |
|---------|--------|
| **LOAD** | Open the ROM picker (lists all `.ch8` files in `roms/`) |
| **RESET** | Reload the current ROM from the beginning |
| **CPU** + **SET** | Set CPU frequency (100–10000 Hz, default 1000). Click the field, type a value, then press **SET** or Enter |
| **PAL** | Cycle display palettes: Amber, Green, Mono, LCD, Pink |
| **DBG** | Toggle debug overlay |
| **−** / **+** / scale label | Change display scale (8×, 10×, 12×, 16×, 20×, 24×) |
| ROM label (right) | Shows the currently loaded ROM filename |

### ROM picker shortcuts

- **Click** a row to load that ROM
- **Mouse wheel** to scroll the list
- **Up / Down** arrow keys to scroll
- **Escape** to close without loading

## Project structure

```
chip8/
├── CMakeLists.txt
├── README.md
├── roms/                  # Place .ch8 ROM files here
├── src/
│   ├── main.cpp           # Main loop and timing
│   ├── core/
│   │   ├── Chip8.hpp      # CPU, memory, opcodes
│   │   └── Chip8.cpp
│   └── platform/
│       ├── PlatformSDL2.hpp   # SDL2 window, input, audio, UI
│       ├── PlatformSDL2.cpp
│       ├── BitmapFont.hpp     # Built-in bitmap font for UI
│       └── BitmapFont.cpp
└── build/                 # CMake output (generated)
```

## ROMs

ROM files are not tracked in git (see `.gitignore`). Add your own `.ch8` files to `roms/`.

Test ROMs and games are available from community collections, for example:

- [Timendus/chip8-test-roms](https://github.com/Timendus/chip8-test-suite.git)

Useful ROMs for verifying the emulator:

| ROM | Purpose |
|-----|---------|
| `opcode.ch8` | Opcode coverage test |
| `testsuite.ch8` | Extended test suite |
| `Pong.ch8` | Simple game |
| `IBM.ch8` | Classic CHIP-8 logo demo |

## References 
- [Cowgod's Chip-8 Technical Reference v1.0](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)

## License

See repository history for authorship. ROM copyrights belong to their respective authors.
