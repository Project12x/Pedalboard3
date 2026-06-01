# 2026-06-01 P0 UI Polish QA Evidence

Command:

```powershell
& 'C:\Program Files\PowerShell\7\pwsh.exe' -ExecutionPolicy Bypass -File scripts\run_d2_visual_qa.ps1 -OutputName 2026-06-01-p0-ui-polish -UiScalePercent 100
```

Build:

- App: `build\Pedalboard3_artefacts\Release\Pedalboard3.exe`
- Pedalboard UI scale: 100%
- OS display scale observed by capture script: 175%
- Capture summary: `capture-summary.json`

Evidence captured:

- Main shell and dense graph: all five built-in themes plus dense graph workflow.
- Stage Mode: before patch switch and after next-patch switch, including long patch names, next-patch cue, meters, tuner control, navigation, and panic affordance.
- Patch switch: main canvas after next-patch switch.
- Dialogs: Plugin Search, Preferences, NAM Model Browser, and IR Browser across Midnight, Daylight, Synthwave, Deep Ocean, and Forest.

Related scaled evidence:

- `documentation\qa\2026-05-27-d2-scaled-footer` covers normal and narrow footer captures at 125%, 150%, 175%, and 200% Pedalboard UI scale.
- `documentation\qa\2026-05-27-d2-scaled-dialogs` covers normal and narrow dialog captures at 150% and 200% Pedalboard UI scale.

Notes:

- This run was captured after the Stage Mode responsive layout metrics and core token-audit enforcement pass.
- Reference-code-first record: clean-room implementation against the local roadmap and existing project code; no external upstream source was copied or ported.
