# Pedalboard3

A free, open-source VST3 plugin host for live performance and rehearsal workflows.

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![JUCE](https://img.shields.io/badge/JUCE-8.x-orange.svg)](https://juce.com)

---

## About

Pedalboard3 is a modular VST3 host for guitarists, keyboardists, and live players who need fast routing, dependable patch switching, and direct control of plugin chains.

Originally created by [Niall Moody](http://www.niallmoody.com) as Pedalboard2, this project has been modernized by Eric Steenwerth for current plugin ecosystems and development tooling.

---

## Current Project Status

Current focus: **Stability and polish**  
Last updated: **February 2026**

### Recently Completed

- Effect Rack feature (`SubGraphProcessor`) with nested plugin hosting
- Stage Mode fullscreen performance view
- Plugin protection foundation:
  - `PluginBlacklist`
  - `CrashProtection` (SEH wrappers + watchdog)
  - Blacklist management UI
- Expanded test coverage (subgraph, cables, integration, protection)
- Monolith sharding of major source files for maintainability

### In Progress

- UI polish and workflow refinement
- Additional testing expansion
- Scanner robustness improvements (timeout and isolation follow-up)

For detailed technical state, see `STATE_OF_PROJECT.md` and `CHANGELOG.md`.

---

## Core Features

- Modular drag-and-drop patching with visual cable routing
- Native VST3 hosting (64-bit)
- MIDI and OSC mapping for plugin parameter control
- Multi-patch setlist workflow with quick switching
- Undo/redo for graph operations
- Built-in themes (Midnight, Daylight, Synthwave, Deep Ocean, Forest)
- Plugin favorites, recent plugins, categorized plugin menu, and search
- Stage Mode for gig use
- Panic command for immediate audio stop

---

## Built-in Processors

- Audio Input / Audio Output
- MIDI Input / MIDI utility processors
- File Player
- MIDI File Player
- Audio Recorder
- Looper
- Metronome
- VU Meter
- Tuner
- Tone Generator
- Routing processors (split/mix)
- Notes and Label nodes
- IR Loader
- NAM processor integration

---

## Installation

### Windows (64-bit)

1. Download the latest release from the GitHub Releases page.
2. Extract `Pedalboard3.exe` to your preferred location.
3. Run the app and scan plugins from **Options -> Plugin List**.

### Build From Source

Requirements:

- Visual Studio 2022 (Windows) or Xcode 14+ (macOS)
- CMake 3.24+
- Git

```bash
git clone --recursive https://github.com/Project12x/Pedalboard3.git
cd Pedalboard3
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Output binary location:

- `build/Pedalboard3_artefacts/Release/`

---

## Quick Start

1. Double-click the plugin field to add processors/plugins.
2. Drag from output pins to input pins to create connections.
3. Use the patch controls at the bottom bar to switch patches.
4. Use `F11` to enter Stage Mode.

---

## Settings Location

| OS | Path |
|----|------|
| Windows 10/11 | `%APPDATA%\Pedalboard3` |
| macOS | `~/Library/Application Support/Pedalboard3` |

---

## Documentation and Planning

- Architecture: `ARCHITECTURE.md`
- Current state: `STATE_OF_PROJECT.md`
- Changelog: `CHANGELOG.md`
- Roadmap: `ROADMAP.md`
- User docs: `documentation/index.html`

---

## Contributing

Contributions are welcome.

1. Fork the repository.
2. Create a feature branch.
3. Submit a pull request with clear change notes and test evidence.

---

## Support

- Report issues via GitHub Issues.
- If you want to support development, see the project funding links in repository metadata and website references.

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

