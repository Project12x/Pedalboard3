# Stage Remix Polish QA

Purpose: verify the first Stage Mode mockup-polish pass after adding the native live strip, current/next patch hierarchy, progress dots, tuner panel, and safety bar treatment.

Command:

```powershell
& 'C:\Program Files\PowerShell\7\pwsh.exe' -ExecutionPolicy Bypass -File scripts\run_d2_visual_qa.ps1 -OutputName 2026-06-09-stage-remix-polish -UiScalePercent 100
```

Build under test: `build\Pedalboard3_artefacts\Release\Pedalboard3.exe`

Capture notes:

- Pedalboard UI scale: 100 percent.
- OS DPI reported by the capture harness: 175 percent.
- Capture summary: `capture-summary.json`.
- The standard theme/dialog capture set was refreshed by the visual QA script.

Stage review notes:

- `workflow-stage-mode-before-switch.png` keeps the top live strip, tuner, exit, previous/next patch controls, current patch name, next-patch cue, progress dots, tuner panel, safety bar, IN/OUT meters, gain sliders, and panic button visible.
- `workflow-stage-mode-after-patch-next.png` verifies the patch-switch state: updated patch count, active progress dot, end-of-set cue, and unchanged access to the same live controls.
- The mockup was used as a visual/hierarchy reference only. No prototype source or assets were copied.
