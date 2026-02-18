# Pedalboard3

A free, open-source VST3 plugin host for live performance and studio workflows.

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![JUCE](https://img.shields.io/badge/JUCE-8.x-orange.svg)](https://juce.com)

---

## What It Is

Pedalboard3 is a standalone application that lets you load VST3 plugins, wire them together on a visual canvas, and switch between patches for live performance or practice. Think of it as a modular plugin host — you build signal chains by dragging cables between nodes, then save and recall them as patches in a setlist.

It started as [Niall Moody's Pedalboard2](http://www.niallmoody.com) (2011-2013), a simple VST2 host built on JUCE 1.x. That project was abandoned after version 2.14. This fork modernizes the entire codebase for current plugin formats and adds the features you'd actually need to use it on stage or in a studio.

---

## Features

### Plugin Hosting
- Native 64-bit VST3 hosting
- Out-of-process plugin scanner with timeout protection — a crashing plugin won't take down the host
- Automatic plugin blacklisting with crash recovery
- Plugin search, favorites, recent plugins, and categorized menus
- Effect Rack — nest an entire plugin chain inside a single node

### Neural Amp Modeling
- Built-in NAM processor for loading `.nam` neural amp models
- Integrated Tone3000 browser for searching, previewing, and downloading models
- OAuth login and background download management with local caching

### Canvas & Navigation
- Drag-and-drop plugin wiring with bezier cables
- Zoom (scroll wheel), pan (click-drag), and fit-to-screen
- Undo/redo for all graph operations
- Smooth viewport scrolling with generous padding

### Live Performance
- Stage Mode (F11) — fullscreen view with large patch name, next-patch preview, and VU meters
- Setlist management with drag-and-drop reordering
- Glitch-free patch switching via crossfade mixer
- Background plugin preloading for instant transitions

### Mixing & Metering
- DAW-style mixer node — N strips with gain faders, pan knobs, mute/solo/phase, stereo toggle
- DAW splitter node — 1 stereo input to N outputs with per-strip controls
- Master bus insert rack for global effects processing
- Per-channel master gain controls (up to 16 channels)
- Professional VU meters with gradient fill, peak hold, dB scale ticks, and glow

### MIDI
- MIDI CC mapping with learn mode, custom ranges, latch/toggle, channel filtering
- MIDI file player with transport, tempo control, and loop
- Transpose, rechannelize, and keyboard split processors
- Virtual MIDI keyboard for testing without hardware
- Lock-free FIFO dispatch — all MIDI parameter control is RT-safe

### Built-in Processors
- Audio Input / Output (multi-channel, with per-channel gain and VU)
- Chromatic tuner (needle + strobe modes, YIN detection)
- Tone generator (sine, square, saw, triangle, noise)
- Oscilloscope
- File player, looper, metronome, audio recorder
- A/B splitter and mixer for parallel routing
- Notes node (markdown rendered) and label node
- IR loader for cabinet simulation

### Themes
- 5 built-in colour schemes: Midnight, Daylight, Synthwave, Deep Ocean, Forest
- Semantic colour token system for consistent theming across the entire UI
- Inter font family for clean typography

---

## Installation

### Windows (64-bit)

1. Download the latest release from the GitHub Releases page.
2. Extract `Pedalboard3.exe` to your preferred location.
3. Run the app and scan plugins from **Options > Plugin List**.

### Build From Source

Requirements:
- Visual Studio 2022 (Windows)
- CMake 3.24+
- Git

```bash
git clone --recursive https://github.com/Project12x/Pedalboard3.git
cd Pedalboard3
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Output: `build/Pedalboard3_artefacts/Release/`

---

## Quick Start

1. Double-click the canvas to add a plugin or processor.
2. Drag from an output pin to an input pin to create a connection.
3. Use the patch bar at the bottom to save, load, and switch patches.
4. Press F11 for Stage Mode.

---

## Settings Location

| OS | Path |
|----|------|
| Windows 10/11 | `%APPDATA%\Pedalboard3` |
| macOS | `~/Library/Application Support/Pedalboard3` |

---

## Project Status

Active development, currently in the stability and polish phase. The core feature set is complete. Remaining work is UI refinement, distribution packaging, and documentation.

For details, see:
- [CHANGELOG.md](CHANGELOG.md) — what changed and when
- [ROADMAP.md](ROADMAP.md) — what's planned
- [ARCHITECTURE.md](ARCHITECTURE.md) — how it's built
- [STATE_OF_PROJECT.md](STATE_OF_PROJECT.md) — current status snapshot

---

## Testing

155 test cases with over 2 million assertions covering signal path, plugin lifecycle, MIDI mapping, cable wiring, graph management, and mutation testing. Tests run via Catch2:

```bash
.\build\tests\Release\Pedalboard3_Tests.exe
```

---

## Contributing

Contributions are welcome.

1. Fork the repository.
2. Create a feature branch.
3. Submit a pull request with clear change notes and test evidence.

---

## Credits

- Original Author: [Niall Moody](http://www.niallmoody.com) (2011)
- Modernization: Eric Steenwerth (2024-2026)
- Framework: [JUCE](https://juce.com)

---

## License

GPL v3. See `license.txt`.

Source code is licensed under the GNU General Public License v3.0.
Original graphics and sounds are under the Creative Commons Sampling Plus license.
