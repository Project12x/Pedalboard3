# 2026-06-13 Node Polish QA

Purpose: compare isolated mockup nodes against isolated Pedalboard app nodes. These are design-review artifacts, not automated pass/fail screenshots.

Generated with:

```powershell
.\scripts\run_d2_visual_qa.ps1 -OutputName 2026-06-13-node-polish -NodeSnapshotsOnly
```

Captured pairs:
- `mockup-node-nam-loader.png` vs. `app-node-nam-loader.png`
- `mockup-node-ir-loader.png` vs. `app-node-ir-loader.png`
- `mockup-node-effect-rack.png` vs. `app-node-effect-rack.png`
- `mockup-node-tuner.png` vs. `app-node-tuner.png`
- `mockup-node-mixer.png` vs. `app-node-mixer.png`
- `mockup-node-splitter.png` vs. `app-node-splitter.png`

Findings:
- NAM Loader and IR Loader still read as embedded editor panels in the app. The mockup uses compact bespoke chassis cards; more resizing of the embedded editors is the wrong direction.
- Effect Rack is closer structurally, but the app rack still needs the mockup's tighter sub-graph badge, nested graph proportions, and compact footer treatment.
- Tuner, Mixer, and Splitter preserve real controls but remain visually less resolved than the mockup node treatments.
- Source/headless tests should remain behavior guardrails only. Visual progress should be reviewed from these node-only PNG pairs before full-window screenshots are used.
