# ReverbSC Internal Node Design

Date: 2026-06-19

## Goal

Add a built-in stereo reverb effect node based on Sean Costello's ReverbSC algorithm. This is a focused DSP/internal-node feature, not part of the RT hosting sprint and not a plugin-format expansion.

The node should give Pedalboard3 a lightweight, reliable built-in late reverb that works without third-party plugins, restores through patch save/load, and follows the same realtime safety expectations as other internal processors.

## Reference-Code-First Record

Primary source inspected:

| Source | Commit | License | Files inspected | Reuse mode |
| --- | --- | --- | --- | --- |
| Csound `reverbsc` | `2932c7fd14681493b5db83df3efdda175c1eb116` | LGPL-2.1-or-later per file header | `Opcodes/reverbsc.c`, `COPYING` | close-port reference |
| Soundpipe `revsc` | `3efb43bdabd0ed23b17c694292b5a79f1692a3ea` | MIT | `modules/revsc.c`, `h/revsc.h`, `LICENSE` | close-port reference |

The repository is GPLv3, so the Csound LGPL lineage and Soundpipe MIT adaptation are compatible with this project. The implementation should preserve attribution to Sean Costello, Istvan Varga, Csound, and Soundpipe/Paul Batchelor in source comments and `THIRD_PARTY_LICENSES.md`.

No external dependency should be added. The node should be a local C++ port shaped for Pedalboard3's architecture.

## User-Facing Shape

Name: `ReverbSC`

Category: `Effects`

I/O:

- Stereo audio input.
- Stereo audio output.
- No MIDI input or output.

Initial parameters:

| Parameter | Range | Default | Notes |
| --- | --- | --- | --- |
| Mix | 0.0-1.0 | 0.35 | Linear dry/wet blend: `0.0` is dry only, `1.0` is wet only. |
| Feedback | 0.0-0.99 | 0.97 | Clamped below runaway unless a separate freeze mode is later added. |
| Damping | 200 Hz-20 kHz | 10000 Hz | Maps to the ReverbSC low-pass frequency. |
| Width | 0.0-1.0 | 1.0 | Stereo width applied after wet output generation. |
| Output | 0.0-2.0 | 1.0 | Post-mix output trim. |

Do not add shimmer, tempo sync, ducking, modulation mode menus, freeze, or predelay in v1. Those are separate effects/features.

## DSP Architecture

Create a portable DSP core with no JUCE dependency:

- `src/dsp/ReverbSC.h`
- `src/dsp/ReverbSC.cpp`

Core API:

```cpp
class ReverbSC
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset() noexcept;
    void setFeedback(float value) noexcept;
    void setDampingHz(float value) noexcept;
    void process(const float* inL, const float* inR, float* outL, float* outR, int numSamples) noexcept;
};
```

Implementation requirements:

- Eight delay lines using the Costello/Varga ReverbSC delay table and random line-segment modulation.
- Allocate delay buffers only in `prepare`.
- No heap allocation, logging, locks, file I/O, settings access, or graph mutation in `process`.
- Recalculate damping coefficient only when damping changes.
- Clamp invalid input parameters and prevent NaN/Inf propagation.
- Clear all delay/filter state in `reset`.

## JUCE/Internal Node Architecture

Add a JUCE wrapper:

- `src/ReverbSCProcessor.h`
- `src/ReverbSCProcessor.cpp`

The wrapper should derive from `PedalboardProcessor`. It should:

- Expose JUCE parameters through the existing internal processor style.
- Call the DSP core from `processBlock`.
- Apply mix, width, and output gain in the processor layer or a small adapter layer.
- Save and restore parameter state through `getStateInformation` / `setStateInformation`.
- Report a stable `PluginDescription` so graph save/load and `InternalPluginFormat` restore work.

Register in:

- `src/InternalFilters.h`
- `src/InternalFilters.cpp`
- `CMakeLists.txt`
- `tests/CMakeLists.txt`

## Testing

Use test-first implementation.

Add focused tests, likely in `tests/reverbsc_processor_test.cpp`:

1. Silence remains silence.
2. A stereo impulse produces a finite decaying tail.
3. Output contains no NaN/Inf under high feedback and high damping.
4. The core works at 44100, 48000, 96000, and 192000 Hz.
5. The core works with block sizes 1, odd sizes, and a typical large block.
6. Feedback and damping changes are clamped and do not destabilize output.
7. Processor state round-trips parameter values.
8. `InternalPluginFormat` lists and instantiates `ReverbSC`.

If source-structure RT guards are added, they should be limited to checking that `processBlock` does not allocate/log and that delay allocation lives in `prepare`.

## Acceptance Criteria

- `ReverbSC` appears as an internal effect node.
- Patch save/load restores the node and its parameters.
- Tests cover core DSP safety, processor state round-trip, and internal format registration.
- Debug test build passes.
- Debug app build passes.
- Attribution/license record is added in the same implementation commit or a paired docs commit.

## Explicit Non-Goals

- No new plugin formats.
- No changes to legacy processor compatibility.
- No replacement of IR loader or NAM cab/reverb behavior.
- No broad UI redesign.
- No RT hosting sprint changes unless a build/test integration issue requires a narrowly scoped fix.
