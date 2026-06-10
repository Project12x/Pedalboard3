# 2026-06-09 Mockup Reference Render

Purpose: preserve a rendered reference from the design handoff so native JUCE polish can be checked against the actual mockup instead of memory.

Source: `F:/Downloads/pedalboard (Remix)-handoff.zip`, extracted under `releases/design-handoffs/pedalboard-remix/pedalboard-remix`.

Rendered file: `NAM Browser.html`

Output:

- `nam-browser-mockup-msedge.png`

Notes:

- Rendered with local Playwright using Microsoft Edge.
- React, ReactDOM, and Babel were served from local npm dependencies so the handoff HTML could run without external CDN access.
- Reuse mode remains pattern-only / clean-room. No mockup source, assets, or fonts were copied into the native app.
- The handoff font emitted a browser font warning, so typography should be treated as approximate in this screenshot.
