# Pedalboard Remix UI Upgrade Phase

## Source

- Handoff bundle: `F:/Downloads/pedalboard (Remix)-handoff.zip`
- Extracted copy: `releases/design-handoffs/pedalboard-remix/pedalboard-remix`
- Primary open file: `project/Pedalboard 3 Demo.html`
- Imported files inspected: `demo-app.jsx`, `mw2-app.jsx`, `mw2-chrome.jsx`, `mw2-nodes.jsx`, `mw2.css`, `scratch-panel.jsx`, `scratch.css`, `StageMode.jsx`, `StageHud.jsx`, `StageSetlist.jsx`, `StageGrid.jsx`, `stage-shared.jsx`, `stage.css`
- Reuse mode: pattern-only. No prototype source code or assets are copied into the native JUCE app.
- License note: the handoff did not include a license file. Treat it as a design reference, not a code dependency.

## Implementation Order

1. Scratch Capture polish: make the scratch window an instant-capture surface with a hero record control, live elapsed time, RAW/WET capture context, destination visibility, and richer recent-take metadata.
2. Stage Mode polish: preserve the existing fullscreen performance view and add the prototype's clear top-bar/view-switcher/safety-bar model in native JUCE.
3. Main graph polish: port only high-signal chrome and routing ideas after scratch and stage stabilize, especially clearer footer priority, node state affordances, cable weight/glow, and labeled pins.

## Current Phase Scope

The first implementation slice upgrades Scratch Capture because it is already the active ship gate and directly supports the desired workflow: plug in, start Pedalboard, and record scratch ideas within moments. This phase also exposes active recorder context in `ScratchRecorderStatus` so the UI can display elapsed time and RAW/WET session details while capture is running.
