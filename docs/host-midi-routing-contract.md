# Host MIDI Routing Contract

This contract applies to external plugin instances wrapped by `BypassableInstance` in the main graph and subgraphs.

## Channel Filtering

- `midiChannel == 0` means omni: the wrapped plugin receives all incoming MIDI events.
- `midiChannel > 0` means channel filter: the wrapped plugin receives MIDI events whose `MidiMessage::getChannel()` matches the configured channel.
- MIDI events on nonmatching channels are not delivered to the wrapped plugin and are forwarded downstream unchanged.
- The plugin's resulting MIDI buffer after `processBlock` is appended to the downstream buffer. A plugin may consume, transform, or emit MIDI; the host does not promise matching-channel input will remain unchanged after the plugin runs.

## Safety Broadcasts

The host treats these messages as safety broadcasts:

- All Notes Off
- All Sound Off
- Reset All Controllers

Safety broadcasts are delivered to the wrapped plugin even when their MIDI channel does not match the wrapper channel filter. They are also forwarded downstream unchanged so later plugins can receive the same panic/control reset even if an earlier plugin consumes its input buffer.

## OSC MIDI Injection

MIDI injected through `BypassableInstance::addMidiMessage` is queued through the wrapper's `MidiMessageCollector` and delivered only to that wrapped plugin. It is not treated as graph input and is not automatically forwarded downstream.

## Realtime Constraints

Routing happens inside `processBlock`, so it must not perform logging, file I/O, UI work, settings lookup, scanner IPC, or blocking synchronization. The current implementation still uses per-block `MidiBuffer` staging consistent with the existing wrapper design; if MIDI pressure becomes a measured realtime issue, this contract should be preserved while replacing the staging buffers with preallocated storage.
