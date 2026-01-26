# Pedalboard3

**A free, open-source VST3 plugin host for live performance.**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![JUCE](https://img.shields.io/badge/JUCE-8.x-orange.svg)](https://juce.com)

---

## About

Pedalboard3 is a modular VST3 plugin host designed for guitarists, keyboardists, and live performers who need reliable, real-time control over their plugin chains. Originally created by [Niall Moody](http://www.niallmoody.com) as "Pedalboard2", it has been modernized and updated by Eric Steenwerth to work with modern audio plugins and development practices.

### Key Features

- **Modular Patching** – Drag-and-drop plugin routing with visual connections
- **VST3 Support** – Native 64-bit VST3 plugin hosting
- **MIDI/OSC Control** – Map any plugin parameter to MIDI CCs or OSC messages
- **Patch Switching** – Queue unlimited patches and switch instantly during performance
- **Undo/Redo** – Full undo support for connection and plugin changes
- **Theme System** – 5 built-in themes (Midnight, Daylight, Synthwave, Deep Ocean, Forest)
- **Background Scanning** – Non-blocking plugin discovery
- **Panic Button** – Instantly stop all audio (Edit → Panic or Ctrl+Shift+P)

---

## Screenshots

*Coming soon*

---

## Installation

### Windows (64-bit)

1. Download the latest release from [Releases](https://github.com/YOUR_USERNAME/Pedalboard3/releases)
2. Extract `Pedalboard3.exe` to your preferred location
3. Run and scan for your VST3 plugins via **Options → Plugin List**

### Building from Source

**Requirements:**
- Visual Studio 2022 (Windows) or Xcode 14+ (macOS)
- CMake 3.24+
- Git

```bash
git clone --recursive https://github.com/YOUR_USERNAME/Pedalboard3.git
cd Pedalboard3
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The executable will be in `build/Pedalboard3_artefacts/Release/`.

---

## Usage

### Quick Start

1. **Add Plugins** – Double-click in the plugin field to open the processor menu
2. **Make Connections** – Click and drag from output pins to input pins
3. **Delete Connections** – Select a connection and press Delete
4. **Switch Patches** – Use the patch dropdown or Next/Prev buttons

### MIDI & OSC Mapping

- Open **Options → Key Mappings** to assign keyboard shortcuts
- All plugin parameters can be controlled via MIDI CCs or OSC
- Configure OSC input in **Options → Misc Settings**

### Built-in Processors

- **Audio Input/Output** – Route system audio
- **MIDI Input/Output** – Route MIDI events
- **File Player** – Play audio files with transport sync
- **Audio Recorder** – Record output to WAV
- **Looper** – Live loop recording/playback
- **Metronome** – Click track with tempo control

---

## Settings Location

| OS | Path |
|----|------|
| Windows 10/11 | `%APPDATA%\Pedalboard3` |
| macOS | `~/Library/Application Support/Pedalboard3` |

---

## Roadmap

See [PHASED_PLAN.md](PHASED_PLAN.md) for the development roadmap including:

- **Phase 4** (Current) – v3.0 Community release
- **Phase 5** – Setlist management, input device support, stage mode
- **Phase 6** – Visual polish, cross-platform builds

---

## Contributing

Contributions are welcome! This is a GPL v3 project.

1. Fork the repository
2. Create a feature branch
3. Submit a pull request

---

## Support

If you find Pedalboard3 useful, consider supporting development:

- ⭐ Star this repo
- 🐛 Report bugs via [Issues](https://github.com/YOUR_USERNAME/Pedalboard3/issues)
- ☕ [Buy me a coffee](https://ko-fi.com/YOUR_USERNAME) *(coming soon)*

---

## Credits

- **Original Author:** [Niall Moody](http://www.niallmoody.com) (2011)
- **Modernization:** Eric Steenwerth (2024-2026)
- **Framework:** [JUCE](https://juce.com) by Raw Material Software

---

## License

**GPL v3** – See [LICENSE](LICENSE) for details.

Source code is licensed under the GNU General Public License v3.0.  
Original graphics and sounds under the Creative Commons Sampling Plus license.

---

*Last updated: January 2026*
