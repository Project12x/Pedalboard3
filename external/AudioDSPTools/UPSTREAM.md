# Upstream provenance

- Project: AudioDSPTools
- Source: https://github.com/sdatkinson/AudioDSPTools
- Base release: `v0.0.0`
- Base commit: `37f73d9ea6698b1b56c5a54fbdfa453a84fbd0b9`
- License: MIT

This is the legacy Pedalboard3 integration snapshot already used by the
application. It is based on AudioDSPTools `v0.0.0` with the existing NAM
plugin integration changes, including the bundled `ResamplingContainer`
headers. It was committed without upgrading it so Windows behavior and legacy
NAM compatibility remain unchanged during the macOS port.
