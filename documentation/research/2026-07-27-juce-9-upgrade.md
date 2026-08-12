# JUCE 9 Upgrade Record

## Dependency Update

- **Upstream:** https://github.com/juce-framework/JUCE
- **Release:** `9.0.0`
- **Pinned commit:** `f8f8864172464b9adf9eba6101e1f784838d1597`
- **License:** JUCE dual license (AGPLv3 or commercial); Pedalboard3 continues
  to consume JUCE under AGPLv3.
- **Reuse mode:** vendored dependency update; no upstream source was copied
  into Pedalboard3-owned files.

## Upstream Files Inspected

- `CMakeLists.txt` — JUCE 9 requires parent CMake projects to enable the C
  language as well as C++.
- `BREAKING_CHANGES.md` — migrated editor creation to
  `createEditorAndMakeActive()` and in-memory SVG parsing to
  `Drawable::createFromSVGString()`.
- `modules/juce_core/juce_core.h` — confirmed the framework's C++17 minimum.
- `LICENSE.md` — retained the upstream licensing terms and attribution.

## Local Compatibility Changes

The top-level project now declares `LANGUAGES C CXX`. SVG loading no longer
round-trips through `juce::XmlElement`, and external/internal plugin editor
windows use JUCE's active-editor API. Windows multi-touch remains disabled by
the JUCE 9 default because Pedalboard3 does not currently implement a
touch-specific interaction path. The embedded Space Grotesk variable font
replaces the legacy static Regular file, which Windows rejects and JUCE 9's
memory-font path does not safely handle. All `Font::getStringWidth*()` calls
now use `GlyphArrangement::getStringWidth()`, which measures shaped text using
JUCE 9's supported API.
