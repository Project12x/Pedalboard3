# Non-AI Feature and Library Research Record

Date: 2026-06-04

## Purpose

This document preserves the research trail from a feature/library exploration session for Pedalboard3. It is not an implementation plan and should not be treated as a recommendation to add dependencies.

The useful conclusion was negative: library-first research produced mostly obvious dependencies, bloated subsystem suggestions, or primitives without a compelling product pull. Future research should start from a genuinely novel live-performance feature and only then evaluate whether a library is necessary.

## Project Constraints Used

- Pedalboard3 is a JUCE 8 / C++17 live audio plugin host.
- Current build uses CMake with CPM/FetchContent and local external sources.
- Existing dependency surface already includes JUCE, fmt, spdlog, nlohmann_json, md4c, Eigen, melatonin_blur, NeuralAmpModelerCore, and AudioDSPTools.
- The app already has VST3 hosting, NAM/IR support, graph routing, MIDI/OSC mappings, Stage Mode, patch switching, internal processors, and UI scale/theming work.
- The codebase and About page identify GPLv3 heritage; license compatibility still matters, especially if commercial distribution goals change.
- User explicitly requested non-AI research.

## Stronger Rule Learned

Do not add "interesting" libraries or primitives unless they are pulled by a specific, musically useful, novel feature.

A primitive without a clear feature is still bloat. A dependency that merely maps to a roadmap checkbox is not necessarily product direction. Research should evaluate feature uniqueness first, implementation mechanics second, libraries last.

## Pass 1: Practical Roadmap Dependency Shortlist

This pass was too conservative. It mostly mapped planned roadmap items to known libraries.

| Candidate | Link | Why Suggested | Current Assessment |
|---|---|---|---|
| sfizz | https://github.com/sfztools/sfizz | C++ SFZ parser/synth library for roadmap item `5K.1 sfizz integration`. | Practical but not novel. Keep only if SFZ sampler becomes a committed feature. |
| Ableton Link | https://github.com/Ableton/link | Network tempo/phase synchronization for roadmap item `5I.8 Ableton Link`. | Useful integration, not cutting edge. GPL/commercial license issue must be handled. |
| sentry-native | https://github.com/getsentry/sentry-native | Native crash reporting for roadmap item `7C.2 Cloud reports`. | Product plumbing, not feature research. |
| WinSparkle | https://github.com/vslavik/winsparkle | Windows auto-update for roadmap item `7B.5 Auto-update`. | Distribution plumbing, not feature research. |
| FluidSynth | https://github.com/FluidSynth/fluidsynth | SF2 synth fallback if SoundFont support matters. | Heavy and LGPL; secondary at best behind a real SF2 requirement. |
| free-audio/clap-juce-extensions | https://github.com/free-audio/clap-juce-extensions | Investigated for CLAP expansion. | Not suitable for Pedalboard hosting; it targets building CLAP plugins from JUCE, not JUCE-based CLAP hosting. |

User feedback: be more experimental and cutting edge, and avoid AI features.

## Pass 2: Experimental Non-AI Dependency Shortlist

This pass was more current but still too dependency-led. Many candidates were full subsystems or architecture commitments.

| Candidate | Link | Why Suggested | Current Assessment |
|---|---|---|---|
| libremidi | https://github.com/celtera/libremidi | Modern MIDI 1.0 / MIDI 2.0 / UMP and WebMIDI-aware I/O. | Interesting but still a broad replacement/integration question. Needs a specific MIDI 2.0 feature before adoption. |
| signalsmith-stretch | https://github.com/Signalsmith-Audio/signalsmith-stretch | Real-time pitch/time stretching for backing tracks or tempo-following audio. | Feature-capable, but should only be considered if tempo-adaptive audio becomes a real product bet. |
| libossia | https://github.com/ossia/libossia | OSCQuery/distributed object model for discoverable remote control. | Likely too much subsystem weight. LGPL-3.0 and conceptual bulk make it a poor default fit. |
| google/highway | https://github.com/google/highway | Portable SIMD with runtime dispatch for DSP acceleration. | Powerful but not feature-led. Use only for a measured hotspot. |
| mjansson/mdns | https://github.com/mjansson/mdns | Tiny mDNS/DNS-SD for local discovery. | Small and sane if a remote/headless feature needs discovery. Still not a feature by itself. |
| lv2/lilv | https://github.com/lv2/lilv | Native LV2 host library. | Practical infrastructure if LV2 hosting becomes active. Not novel product direction. |
| google/dawn | https://github.com/google/dawn | Native WebGPU for GPU visual nodes. | Too large for speculative exploration. Only valid for a concrete visual performance surface. |
| floooh/sokol | https://github.com/floooh/sokol | Lightweight GPU abstraction alternative to Dawn. | Less bloated than Dawn, but still unjustified without a specific visual feature. |

User feedback: these still felt like bloat rather than cutting edge.

## Pass 3: Small Primitive / Build-Time Tool Shortlist

This pass attempted to avoid subsystem bloat by focusing on micro-libraries, single-header tools, or build-time generators. The correction from user feedback is that even these are bloat without feature pull.

| Candidate | Link | Why Suggested | Current Assessment |
|---|---|---|---|
| Faust | https://github.com/grame-cncm/faust | Build-time DSP language/compiler for generating experimental internal processors. | Best of this pass only if tied to a specific new processor concept. Do not ship compiler as runtime. |
| Signalsmith Audio DSP | https://github.com/Signalsmith-Audio/dsp | Header-only FFT, STFT, filters, windows, envelopes, spectral helpers. | Useful source of small DSP pieces, but avoid adding without a feature. |
| r8brain-free-src | https://github.com/avaneev/r8brain-free-src | Header-only high-quality sample-rate conversion. | Sane if sample playback, asset prep, or resampling quality becomes a real need. |
| free-audio/clap | https://github.com/free-audio/clap | CLAP protocol headers/reference. | Use as reference material for a thin spike, not as a vague "CLAP support" dependency. |
| Tracktion/choc | https://github.com/Tracktion/choc | Header-only utility collection from Tracktion. | Do not add wholesale. At most inspect one header for a missing primitive. |
| DaisySP | https://github.com/electro-smith/DaisySP | MIT DSP building blocks from embedded synth ecosystem. | Interesting source-study material, but adopting a DSP grab-bag would be bloat. |
| SOUL | https://github.com/soul-lang/SOUL | Audio programming language/runtime. | Too stale and too architectural for this project. Not recommended. |
| Tracy | https://github.com/wolfpld/tracy | Frame/profiling tool. | Good engineering tool, but not product research. Roadmap already mentions Tracy profiler. |
| AudioFFT | https://github.com/HiFi-LoFi/AudioFFT | Small real FFT library. | Stale; Signalsmith DSP or JUCE FFT are more plausible. |
| Mutable Instruments stmlib / eurorack | https://github.com/pichenettes/stmlib and https://github.com/pichenettes/eurorack | Source-study/reference for distinctive synthesis/DSP ideas. | Reference only. License and source-copy implications require care. |

User feedback: primitives for weird features with no ideas for those features are still bloat.

## Pass 4: Feature Brainstorming

This pass moved from libraries to features, but most ideas were still not novel or musically compelling enough.

| Feature Idea | Why Suggested | Current Assessment |
|---|---|---|
| Patch Morphing | Morph between two patches or rig states. | Rejected by user as not musically useful. Likely demo-friendly but weak live utility. |
| Gesture Recorder | Record footswitches, MIDI CC, patch changes, bypass toggles, and slider moves as replayable performance lanes. | Not obviously novel; risks becoming a DAW-lite automation lane. |
| Signal-Aware Cables | Show activity, clipping, MIDI density, silence, bypass state on graph cables. | Diagnostic value, but not enough as a product-defining feature. |
| Freeze / Drone Node | Capture live input into an ambient pad and route it through the graph. | Potentially musical, but not sufficiently explored; could overlap common freeze pedals. |
| Latency / Phase Probe | Probe chains, measure latency/phase, and compensate. | Technically useful for parallel routing and hardware loops, but utility/tooling rather than novel creative direction. |
| Scene Rules | Rule-based transitions such as enabling effects, changing labels, or raising output for song sections. | Useful but not novel; could become brittle workflow automation. |
| Cable Looper / Signal Tap | Turn any cable into a rolling recorder/audition point. | More graph-native than most suggestions, but still needs stronger musical framing. |
| Virtual Soundcheck for Every Patch | Run a dry DI phrase through every patch and report silence, clipping, CPU spikes, missing files, bad routing, and level jumps. | Useful QA feature, but not novel enough for the requested bar. |
| Audio-to-Control Cables | Convert envelope/transient density/pitch/brightness/noise floor from any cable into parameter control. | Closest to a graph-native creative feature, but still needs a sharper musical use case. |
| Feedback Sentinel | Detect runaway feedback/DC/subsonic/extreme resonance per branch and clamp or mute only that path. | Practical live safety idea, but not enough as "cutting edge." |
| Hardware Loop Calibrator | Probe external hardware loops and compensate latency/phase/level/EQ. | Useful for hybrid rigs, but more calibration utility than novel performance feature. |

User feedback: almost all were terrible; not novel or interesting enough.

## Research Takeaways

1. Roadmap-mapped libraries are useful implementation notes, not product research.
2. "Cutting edge" cannot mean "newer dependency" or "modern protocol" by itself.
3. Small primitives still add conceptual weight when they do not serve a specific feature.
4. The most promising direction is likely not another host feature, format, sampler, updater, or control API.
5. Feature research should begin with musician behavior and live-performance friction that ordinary hosts cannot address.
6. Any future candidate should be rejected unless it passes all of these gates:
   - It is musically useful in an actual live set.
   - It is graph-native or stage-native, not generic DAW/host functionality.
   - It changes what a user can do, not just how the app is implemented.
   - It can be prototyped mostly with existing code.
   - It has a clear "why Pedalboard?" answer.

## Suggested Next Research Method

Do not search for libraries next.

Instead, write 10 to 20 one-paragraph feature concepts using this format:

```text
Feature:
Live situation:
What the musician does:
What Pedalboard does that a normal host does not:
Why it is musically useful:
What existing Pedalboard systems it uses:
Why it might be a bad idea:
```

Only after a concept survives taste review should library research resume.

## Source Links Consulted

- https://github.com/sfztools/sfizz
- https://github.com/Ableton/link
- https://github.com/getsentry/sentry-native
- https://github.com/vslavik/winsparkle
- https://github.com/FluidSynth/fluidsynth
- https://github.com/free-audio/clap-juce-extensions
- https://github.com/celtera/libremidi
- https://github.com/Signalsmith-Audio/signalsmith-stretch
- https://github.com/ossia/libossia
- https://github.com/google/highway
- https://github.com/mjansson/mdns
- https://github.com/lv2/lilv
- https://github.com/google/dawn
- https://github.com/floooh/sokol
- https://github.com/grame-cncm/faust
- https://github.com/Signalsmith-Audio/dsp
- https://github.com/avaneev/r8brain-free-src
- https://github.com/free-audio/clap
- https://github.com/Tracktion/choc
- https://github.com/electro-smith/DaisySP
- https://github.com/soul-lang/SOUL
- https://github.com/wolfpld/tracy
- https://github.com/HiFi-LoFi/AudioFFT
- https://github.com/pichenettes/stmlib
- https://github.com/pichenettes/eurorack

