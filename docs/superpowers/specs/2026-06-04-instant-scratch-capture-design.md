# Instant Scratch Capture Design

Date: 2026-06-04

## Status

Approved design direction.

This spec captures the agreed first version of a Pedalboard feature for plugging in a guitar, launching Pedalboard, and recording a scratch idea within moments. The design combines a global recorder with a compact scratch panel, while reusing the existing recording backbone patterns instead of adding a new dependency.

## Product Intent

Pedalboard should make idea capture feel as immediate as getting sound. The user should not have to add an Audio Recorder node, choose a file, name a take, configure transport sync, or change the current patch before recording.

The feature is manual in v1: the user starts and stops capture explicitly. Retrospective "save the last N seconds" capture is not part of this version.

## Goals

- Provide one obvious Record/Stop control available from the main Pedalboard UI.
- Record without opening a file picker or modifying the current graph.
- Save every scratch recording automatically as a take bundle.
- Capture both raw DI input and the wet app output at the same time.
- Keep the raw and wet files sample-synchronized so the raw file can be reamped later.
- Show recent scratch takes in a compact panel with quick reveal/open actions.
- Reuse Pedalboard's existing recorder writer pattern where it fits.
- Keep audio callback work real-time safe: no allocation, file creation, XML/JSON writes, dialogs, or locks in the callback.

## Non-Goals

- No retrospective rolling buffer in v1.
- No DAW timeline, arrangement view, overdub system, punch-in flow, comping, or waveform editor.
- No automatic musical naming, tagging, transcription, or AI feature.
- No replacement for the existing Audio Recorder or Looper processors.
- No requirement that scratch capture be represented as a graph node.
- No cloud sync or sharing flow.

## Capture Semantics

Each scratch take is saved as a folder containing:

- `wet.wav`: the stereo or multichannel audio sent to the output device after the Pedalboard graph, master bus insert, and output gain.
- `raw.wav`: the active device input channels before master input gain and before the Pedalboard graph.
- `take.json`: metadata describing the take.

Raw means "the closest DI signal Pedalboard receives from the audio interface." It is intentionally captured before master input gain so later reamping is not affected by the monitoring gain chosen during the session.

Wet means "what the user heard from Pedalboard." It is captured from the final output buffers after graph processing, the master bus insert rack, and master output gain have been applied.

The raw and wet writers start from the same audio callback block and stop on the same block boundary. If raw and wet channel counts differ, both files still share sample rate and sample length.

## User Experience

The main window gets a compact scratch capture control that is visible without opening a plugin editor. The default collapsed form should fit the footer or an adjacent main-panel strip:

- Record/Stop button.
- Elapsed time while recording.
- Status text such as `Ready`, `Recording`, `Saving`, or `Saved`.
- A small indicator that both `Raw` and `Wet` are armed.
- A button to open the scratch panel or reveal the latest take.

The scratch panel is a simple utility surface, not an editor:

- Large Record/Stop control.
- Current take elapsed time and destination folder.
- Recent take list showing timestamp, duration, patch name, and raw/wet availability.
- Reveal folder action.
- Optional "Play wet" action if it can be implemented without turning the panel into a timeline.

Starting capture never asks where to save. Stopping capture saves the take bundle and shows a toast such as `Scratch take saved`.

## Storage

Default folder:

`<Pedalboard user data directory>/Scratch Ideas/`

Take folder naming:

`YYYY-MM-DD/HHMMSS-<patch-or-untitled>/`

The folder name should be sanitized for the filesystem and should not require uniqueness from user-entered patch names. If a collision occurs, append `-02`, `-03`, and so on.

`take.json` should include:

- take id
- start timestamp
- duration in samples and seconds
- sample rate
- raw channel count
- wet channel count
- audio device name
- document path if available
- current patch index and patch name
- master input/output gain values
- raw file path
- wet file path
- app version if available

## Architecture

Add an app-level scratch recorder service and panel:

- `ScratchRecorder`: owns recording state, file writers, take metadata, and audio callback write entry points.
- `ScratchTake`: plain data model for the take bundle and metadata.
- `ScratchPanel`: compact UI for current state and recent takes.
- `MainPanel` integration: creates the recorder, exposes menu/command entries, hosts the compact control, shows toasts, and passes patch/document context to `ScratchRecorder`.
- `MeteringProcessorPlayer` integration: taps raw input buffers before input gain and taps wet output buffers after output gain.

The existing `RecorderProcessor` remains available as a graph processor. The scratch feature should reuse its proven `AudioFormatWriter::ThreadedWriter` approach, ideally by extracting a small reusable writer helper rather than copying the entire processor.

## Data Flow

1. User clicks Record in the main UI or scratch panel.
2. `MainPanel` gathers patch/document/device context and asks `ScratchRecorder` to start a take.
3. `ScratchRecorder` creates the take folder and opens `raw.wav` and `wet.wav` writers on the message thread.
4. On the next audio callback, `MeteringProcessorPlayer` passes the pre-gain input pointers to `ScratchRecorder::writeRawBlock()`.
5. The graph and master bus process normally.
6. After output gain, `MeteringProcessorPlayer` passes output pointers to `ScratchRecorder::writeWetBlock()`.
7. User clicks Stop.
8. `ScratchRecorder` stops accepting blocks at a block boundary, releases writers off the callback, writes `take.json`, and notifies the UI.
9. `MainPanel` refreshes the recent take list and shows a saved toast.

## Real-Time Safety

The audio callback may only:

- read an atomic recording-state flag
- pass existing channel pointers and sample counts into pre-created `ThreadedWriter` instances
- update simple atomic counters for elapsed samples and error state

The audio callback must not:

- allocate memory
- open, close, delete, or rename files
- create JSON/XML
- show dialogs or toasts
- call into UI components
- acquire locks

Writer creation, writer release, metadata write, and panel refresh happen on the message thread or a non-audio worker path.

## Error Handling

If either writer cannot be created, capture does not start and the UI shows a clear failure state.

If one writer fails during capture, the take is marked incomplete and recording stops cleanly. The UI should say which side failed: raw, wet, or both.

If no input channels are active, wet-only capture may still proceed only if the user explicitly allows it. The default v1 behavior is to refuse capture with `No input channels available`, because the feature is specifically for guitar scratch capture and reamping.

If no output channels are active, capture is refused because there is no wet print.

If the audio device changes while recording, the recorder stops at the next safe boundary and marks the take interrupted.

## Commands And Settings

Add application commands:

- `Start/Stop Scratch Capture`
- `Open Scratch Panel`
- `Reveal Scratch Ideas Folder`

Add settings:

- Scratch folder path, defaulting to the Pedalboard user data directory.
- Optional "show scratch control in footer" toggle if the footer becomes too crowded.

The initial implementation should keep the control visible by default because this is a core quick-capture workflow.

## Testing

Unit or integration tests should cover:

- take folder naming and collision handling
- metadata generation
- start failure when raw or wet writer creation fails
- state transitions: ready, recording, stopping, saved, failed
- raw/wet sample count equality for simulated blocks
- safe stop on device change

Manual verification should cover:

- launch Pedalboard, press Record, play guitar, press Stop, and verify both WAV files exist
- verify `raw.wav` contains DI input and `wet.wav` contains processed output
- verify no graph node is added or modified
- verify patch switching while recording is either prevented or stops capture cleanly with an interrupted status
- verify reveal-folder opens the take bundle
- verify capture works after loading an existing `.pdl`

## Reference Code And Reuse Mode

Internal source inspected:

- `src/MainPanel.h`: `MeteringProcessorPlayer` already sees raw device input before gain and wet output after graph/master processing.
- `src/MainPanel.cpp`: owns audio device setup, command/menu integration, footer UI, toast overlay, patch context, and `MeteringProcessorPlayer`.
- `src/RecorderProcessor.cpp`: existing `WavAudioFormat`, `FileOutputStream`, and `AudioFormatWriter::ThreadedWriter` pattern for audio-thread recording.
- `src/AudioRecorderControl.cpp`: existing recorder UI and pending-change polling pattern.
- `P0_GIG_SPEED_FEATURES.md`: confirms the broader product direction of time-to-sound and gig-speed utility.

Reuse mode:

- Internal pattern reuse from `RecorderProcessor`, not external source reuse.
- No new third-party library is required for v1.
- If implementation extracts a reusable writer helper, `RecorderProcessor` and `ScratchRecorder` should both use it only if that reduces duplication without destabilizing the existing recorder.

## Open Product Decisions Resolved For V1

- Capture is manual, not retrospective.
- Capture is global, not a graph node.
- The UI combines an always-reachable compact control with a lightweight scratch panel.
- Every take records both raw and wet files.
- Raw capture is pre-master-input-gain.
- Wet capture is post-chain output.
