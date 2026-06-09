# Main Footer Scale QA - 2026-06-09

Scope: mockup-informed main chrome/footer polish without removing existing footer UX.

Command:

```powershell
& 'C:\Program Files\PowerShell\7\pwsh.exe' -ExecutionPolicy Bypass -File scripts\run_d2_visual_qa.ps1 -OutputName 2026-06-09-main-footer-scale-v2 -CaptureScaledFooterMatrix
```

Result:

- Release app build used: `build\Pedalboard3_artefacts\Release\Pedalboard3.exe`.
- OS scale reported by the QA harness: 175%.
- Captured scaled footer matrix at 125%, 150%, 175%, and 200%, normal and narrow.
- Narrow captures keep the existing footer functions visible: patch selector and patch +/- controls, tempo/transport controls, Scratch `REC` and `Takes`, input/output gain sliders, `FX`, `Manage`, `Fit`, CPU meter, and the UI Scale dropdown.
- CPU footer label was shortened to `CPU` so it stays readable in the scaled footer.

Representative evidence:

- `workflow-scaled-footer-150-narrow.png`
- `workflow-scaled-footer-200-narrow.png`

