param(
    [string]$Configuration = "Release",
    [string]$OutputName = "2026-05-26-d2-visual",
    [int]$ExpectedScalePercent = 0
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
$outputDir = Join-Path $repoRoot "documentation\qa\$OutputName"
$tmpDir = Join-Path $env:TEMP "Pedalboard3VisualQa"
$appDataDir = Join-Path $env:APPDATA "Pedalboard3"
$settingsPath = Join-Path $appDataDir "settings.json"
$settingsBackup = Join-Path $tmpDir "settings.backup.json"
$defaultPatchPath = Join-Path $appDataDir "default.pdl"
$defaultPatchBackup = Join-Path $tmpDir "default.backup.pdl"
$qaPatch = Join-Path $tmpDir "visual-qa.pdl"

$candidateApps = @(
    (Join-Path $repoRoot "build\Pedalboard3_artefacts\$Configuration\Pedalboard3.exe"),
    (Join-Path $repoRoot "build\msvc-release\Pedalboard3_artefacts\$Configuration\Pedalboard3.exe")
)

$appPath = $candidateApps | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $appPath) {
    throw "Could not find Pedalboard3.exe for configuration '$Configuration'."
}

New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
New-Item -ItemType Directory -Force -Path $tmpDir | Out-Null

Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

public static class VisualQaWin32
{
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    public struct WindowInfo
    {
        public IntPtr Handle;
        public string Title;
    }

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("dwmapi.dll")]
    public static extern int DwmGetWindowAttribute(IntPtr hwnd, int dwAttribute, out RECT pvAttribute, int cbAttribute);

    [DllImport("user32.dll")]
    public static extern bool MoveWindow(IntPtr hWnd, int X, int Y, int nWidth, int nHeight, bool bRepaint);

    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [DllImport("user32.dll")]
    public static extern uint GetDpiForWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint processId);

    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern int GetWindowTextLength(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    public static WindowInfo[] GetProcessWindows(uint targetProcessId)
    {
        var windows = new List<WindowInfo>();

        EnumWindows((hWnd, lParam) =>
        {
            if (!IsWindowVisible(hWnd))
                return true;

            uint windowProcessId;
            GetWindowThreadProcessId(hWnd, out windowProcessId);
            if (windowProcessId != targetProcessId)
                return true;

            var length = GetWindowTextLength(hWnd);
            var title = "";
            if (length > 0)
            {
                var builder = new StringBuilder(length + 1);
                GetWindowText(hWnd, builder, builder.Capacity);
                title = builder.ToString();
            }

            windows.Add(new WindowInfo { Handle = hWnd, Title = title });
            return true;
        }, IntPtr.Zero);

        return windows.ToArray();
    }

}
"@

function Write-QaPatch {
    New-Item -ItemType Directory -Force -Path $appDataDir | Out-Null

    @'
<?xml version="1.0" encoding="UTF-8"?>
<Pedalboard3PatchFile>
  <Patch tempo="120.0" name="QA Dense Graph - Long Patch Name For Stage Readability">
    <FILTERGRAPH>
      <FILTER uid="1" x="140.0" y="80.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0">
        <PLUGIN name="Audio Input" descriptiveName="" format="Internal" category="I/O devices" manufacturer="JUCE" version="1.0" file="" uniqueId="246006c0" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="0" numOutputs="2" isShell="0" hasARAExtension="0" uid="246006c0"/>
      </FILTER>
      <FILTER uid="2" x="140.0" y="340.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0">
        <PLUGIN name="MIDI Input" descriptiveName="" format="Internal" category="I/O devices" manufacturer="JUCE" version="1.0" file="" uniqueId="a9e4f9eb" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="0" numOutputs="0" isShell="0" hasARAExtension="0" uid="a9e4f9eb"/>
      </FILTER>
      <FILTER uid="3" x="140.0" y="240.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0">
        <PLUGIN name="Virtual MIDI Input" descriptiveName="Virtual MIDI Keyboard Input" format="Internal" category="MIDI Utility" manufacturer="Pedalboard3" version="1.0" file="VirtualMidiInput" uniqueId="0" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="0" numOutputs="0" isShell="0" hasARAExtension="0" uid="0"/>
      </FILTER>
      <FILTER uid="4" x="1040.0" y="200.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0">
        <PLUGIN name="Audio Output" descriptiveName="" format="Internal" category="I/O devices" manufacturer="JUCE" version="1.0" file="" uniqueId="724248cb" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="2" numOutputs="0" isShell="0" hasARAExtension="0" uid="724248cb"/>
      </FILTER>
      <FILTER uid="10" x="360.0" y="80.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0"><PLUGIN name="Level" descriptiveName="Simple level processor." format="Internal" category="Built-in" manufacturer="Niall Moody" version="1.00" file="" uniqueId="0" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="2" numOutputs="2" isShell="0" hasARAExtension="0" uid="0"/></FILTER>
      <FILTER uid="11" x="580.0" y="80.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0"><PLUGIN name="VU Meter" descriptiveName="Simple VU Meter." format="Internal" category="Built-in" manufacturer="Niall Moody" version="1.00" file="" uniqueId="0" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="2" numOutputs="2" isShell="0" hasARAExtension="0" uid="0"/></FILTER>
      <FILTER uid="12" x="800.0" y="80.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0"><PLUGIN name="IR Loader" descriptiveName="Cabinet Impulse Response Loader" format="Internal" category="Effects" manufacturer="Pedalboard3" version="1.0" file="IR Loader" uniqueId="0" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="2" numOutputs="2" isShell="0" hasARAExtension="0" uid="0"/></FILTER>
      <FILTER uid="13" x="360.0" y="260.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0"><PLUGIN name="Tone Generator" descriptiveName="Test signal generator for tuner and plugin testing" format="Internal" category="Built-in" manufacturer="Pedalboard3" version="1.0" file="tonegenerator" uniqueId="0" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="0" numOutputs="2" isShell="0" hasARAExtension="0" uid="0"/></FILTER>
      <FILTER uid="14" x="580.0" y="260.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0"><PLUGIN name="Tuner" descriptiveName="Chromatic Tuner" format="Internal" category="Built-in" manufacturer="Pedalboard3" version="1.0" file="Tuner" uniqueId="0" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="1" numOutputs="0" isShell="0" hasARAExtension="0" uid="0"/></FILTER>
      <FILTER uid="15" x="800.0" y="260.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0"><PLUGIN name="Oscilloscope" descriptiveName="Oscilloscope" format="Internal" category="Built-in" manufacturer="Pedalboard" version="1.0" file="Oscilloscope" uniqueId="0" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="1" numOutputs="1" isShell="0" hasARAExtension="0" uid="0"/></FILTER>
      <FILTER uid="16" x="360.0" y="480.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0"><PLUGIN name="Mixer" descriptiveName="Mixes two stereo pairs (A and B) to stereo with gain, pan, mute/solo." format="Internal" category="Built-in" manufacturer="Pedalboard3" version="1.0" file="" uniqueId="0" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="4" numOutputs="2" isShell="0" hasARAExtension="0" uid="0"/></FILTER>
      <FILTER uid="17" x="580.0" y="480.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0"><PLUGIN name="Splitter" descriptiveName="Splits stereo input to two stereo pairs (A and B)." format="Internal" category="Built-in" manufacturer="Pedalboard3" version="1.0" file="" uniqueId="0" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="2" numOutputs="4" isShell="0" hasARAExtension="0" uid="0"/></FILTER>
      <FILTER uid="18" x="800.0" y="480.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0"><PLUGIN name="Effect Rack" descriptiveName="" format="Internal" category="Built-in" manufacturer="Pedalboard3" version="1.0" file="Internal:SubGraph" uniqueId="0" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="2" numOutputs="2" isShell="0" hasARAExtension="0" uid="0"/></FILTER>
      <FILTER uid="19" x="140.0" y="560.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0"><PLUGIN name="Notes" descriptiveName="Displays text notes on the canvas." format="Internal" category="Built-in" manufacturer="Eric Steenwerth" version="1.0" file="" uniqueId="0" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="0" numOutputs="0" isShell="0" hasARAExtension="0" uid="0"/></FILTER>
      <CONNECTION srcFilter="1" srcChannel="0" dstFilter="10" dstChannel="0"/>
      <CONNECTION srcFilter="1" srcChannel="1" dstFilter="10" dstChannel="1"/>
      <CONNECTION srcFilter="10" srcChannel="0" dstFilter="11" dstChannel="0"/>
      <CONNECTION srcFilter="10" srcChannel="1" dstFilter="11" dstChannel="1"/>
      <CONNECTION srcFilter="11" srcChannel="0" dstFilter="12" dstChannel="0"/>
      <CONNECTION srcFilter="11" srcChannel="1" dstFilter="12" dstChannel="1"/>
      <CONNECTION srcFilter="12" srcChannel="0" dstFilter="4" dstChannel="0"/>
      <CONNECTION srcFilter="12" srcChannel="1" dstFilter="4" dstChannel="1"/>
    </FILTERGRAPH>
    <Mappings/>
    <UserNames/>
  </Patch>
  <Patch tempo="96.0" name="QA Patch Switch Target">
    <FILTERGRAPH>
      <FILTER uid="1" x="180.0" y="120.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0">
        <PLUGIN name="Audio Input" descriptiveName="" format="Internal" category="I/O devices" manufacturer="JUCE" version="1.0" file="" uniqueId="246006c0" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="0" numOutputs="2" isShell="0" hasARAExtension="0" uid="246006c0"/>
      </FILTER>
      <FILTER uid="2" x="180.0" y="320.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0">
        <PLUGIN name="MIDI Input" descriptiveName="" format="Internal" category="I/O devices" manufacturer="JUCE" version="1.0" file="" uniqueId="a9e4f9eb" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="0" numOutputs="0" isShell="0" hasARAExtension="0" uid="a9e4f9eb"/>
      </FILTER>
      <FILTER uid="4" x="760.0" y="160.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0">
        <PLUGIN name="Audio Output" descriptiveName="" format="Internal" category="I/O devices" manufacturer="JUCE" version="1.0" file="" uniqueId="724248cb" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="2" numOutputs="0" isShell="0" hasARAExtension="0" uid="724248cb"/>
      </FILTER>
      <FILTER uid="30" x="440.0" y="140.0" uiLastX="0" uiLastY="0" windowOpen="0" program="0"><PLUGIN name="Level" descriptiveName="Simple level processor." format="Internal" category="Built-in" manufacturer="Niall Moody" version="1.00" file="" uniqueId="0" isInstrument="0" fileTime="0" infoUpdateTime="0" numInputs="2" numOutputs="2" isShell="0" hasARAExtension="0" uid="0"/></FILTER>
      <CONNECTION srcFilter="1" srcChannel="0" dstFilter="30" dstChannel="0"/>
      <CONNECTION srcFilter="1" srcChannel="1" dstFilter="30" dstChannel="1"/>
      <CONNECTION srcFilter="30" srcChannel="0" dstFilter="4" dstChannel="0"/>
      <CONNECTION srcFilter="30" srcChannel="1" dstFilter="4" dstChannel="1"/>
    </FILTERGRAPH>
    <Mappings/>
    <UserNames/>
  </Patch>
</Pedalboard3PatchFile>
'@ | Set-Content -Path $qaPatch -Encoding UTF8

    Copy-Item -LiteralPath $qaPatch -Destination $defaultPatchPath -Force
}

function Set-QaSettings {
    param([string]$Theme)

    New-Item -ItemType Directory -Force -Path $appDataDir | Out-Null
    $settings = [ordered]@{
        colourScheme = $Theme
        useTrayIcon = $false
        startInTray = $false
        LoopPatches = $true
        pdlAudioSettings = $false
        WindowState = ""
    }

    $settings | ConvertTo-Json -Depth 4 | Set-Content -Path $settingsPath -Encoding UTF8
}

function Wait-MainWindow {
    param([System.Diagnostics.Process]$Process)

    $null = $Process.WaitForInputIdle(10000)
    for ($i = 0; $i -lt 80; ++$i) {
        $Process.Refresh()
        if ($Process.MainWindowHandle -ne [IntPtr]::Zero) {
            return $Process.MainWindowHandle
        }
        Start-Sleep -Milliseconds 250
    }
    throw "Timed out waiting for Pedalboard3 main window."
}

function Get-WindowRect {
    param([IntPtr]$Handle)

    $rect = New-Object VisualQaWin32+RECT
    $dwmResult = [VisualQaWin32]::DwmGetWindowAttribute($Handle, 9, [ref]$rect, [Runtime.InteropServices.Marshal]::SizeOf($rect))
    if ($dwmResult -ne 0) {
        [VisualQaWin32]::GetWindowRect($Handle, [ref]$rect) | Out-Null
    }
    return $rect
}

function Capture-Window {
    param(
        [IntPtr]$Handle,
        [string]$Path,
        [int]$CaptureWidth = 0,
        [int]$CaptureHeight = 0
    )

    [VisualQaWin32]::ShowWindow($Handle, 5) | Out-Null
    if ($CaptureWidth -gt 0 -and $CaptureHeight -gt 0) {
        [VisualQaWin32]::SetWindowPos($Handle, [IntPtr](-1), 40, 40, $CaptureWidth, $CaptureHeight, 0x0040) | Out-Null
    }
    [VisualQaWin32]::SetForegroundWindow($Handle) | Out-Null
    Start-Sleep -Milliseconds 500

    $rect = Get-WindowRect -Handle $Handle
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    if ($width -le 0 -or $height -le 0) {
        throw "Invalid capture bounds ${width}x${height}."
    }

    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $bitmap.Size)
        $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

function Assert-ExpectedScale {
    param([int]$ActualScalePercent)

    if ($ExpectedScalePercent -le 0) {
        return
    }

    if ($ActualScalePercent -ne $ExpectedScalePercent) {
        throw "Expected display scale $ExpectedScalePercent%, but Pedalboard3 reported $ActualScalePercent%. Change Windows Display scale before this exact-DPI capture, or omit -ExpectedScalePercent for exploratory captures."
    }
}

function Get-ProcessWindows {
    param([int]$ProcessId)

    [VisualQaWin32]::GetProcessWindows([uint32]$ProcessId) | ForEach-Object {
        [pscustomobject]@{
            Handle = $_.Handle
            Title = $_.Title
        }
    }
}

function Find-QaWindow {
    param(
        [System.Diagnostics.Process]$Process,
        [string]$Title
    )

    for ($i = 0; $i -lt 80; ++$i) {
        $windows = Get-ProcessWindows -ProcessId $Process.Id
        $match = $windows | Where-Object { $_.Title -eq $Title } | Select-Object -First 1
        if ($match) {
            return $match.Handle
        }

        Start-Sleep -Milliseconds 250
    }

    $known = (Get-ProcessWindows -ProcessId $Process.Id | ForEach-Object { "'$($_.Title)'" }) -join ", "
    throw "Timed out waiting for visual QA window '$Title'. Known windows: $known"
}

function Start-QaApp {
    param(
        [string]$Theme,
        [string[]]$Arguments = @()
    )

    Set-QaSettings -Theme $Theme

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $appPath
    $psi.Arguments = ($Arguments -join " ")
    $psi.WorkingDirectory = Split-Path -Parent $appPath
    $process = [System.Diagnostics.Process]::Start($psi)
    $handle = Wait-MainWindow -Process $process
    [VisualQaWin32]::MoveWindow($handle, 40, 40, 1280, 820, $true) | Out-Null
    [VisualQaWin32]::SetWindowPos($handle, [IntPtr](-1), 40, 40, 1280, 820, 0x0040) | Out-Null
    [VisualQaWin32]::SetForegroundWindow($handle) | Out-Null
    Start-Sleep -Seconds 2

    return [pscustomobject]@{
        Process = $process
        Handle = $handle
    }
}

function Stop-QaApp {
    param([System.Diagnostics.Process]$Process)

    if ($null -eq $Process -or $Process.HasExited) {
        return
    }

    $Process.CloseMainWindow() | Out-Null
    if (-not $Process.WaitForExit(2500)) {
        $Process.Kill()
        $Process.WaitForExit(5000) | Out-Null
    }
}

$existing = Get-Process -Name "Pedalboard3" -ErrorAction SilentlyContinue
if ($existing) {
    throw "Pedalboard3 is already running. Close it before running visual QA so settings can be backed up safely."
}

$settingsExisted = Test-Path $settingsPath
if ($settingsExisted) {
    Copy-Item -LiteralPath $settingsPath -Destination $settingsBackup -Force
}

$defaultPatchExisted = Test-Path $defaultPatchPath
if ($defaultPatchExisted) {
    Copy-Item -LiteralPath $defaultPatchPath -Destination $defaultPatchBackup -Force
}

Write-QaPatch

$captures = New-Object System.Collections.Generic.List[object]
$dpi = $null
$themes = @("Midnight", "Daylight", "Synthwave", "Deep Ocean", "Forest")

try {
    foreach ($theme in $themes) {
        $session = $null
        try {
            $session = Start-QaApp -Theme $theme
            if ($null -eq $dpi) {
                try { $dpi = [VisualQaWin32]::GetDpiForWindow($session.Handle) } catch { $dpi = 96 }
                Assert-ExpectedScale -ActualScalePercent ([Math]::Round(($dpi / 96.0) * 100))
            }

            $safeTheme = $theme.ToLowerInvariant().Replace(" ", "-")
            $path = Join-Path $outputDir ("theme-{0}-main.png" -f $safeTheme)
            Capture-Window -Handle $session.Handle -Path $path -CaptureWidth 1280 -CaptureHeight 820
            $captures.Add([pscustomobject]@{ Name = "theme-$safeTheme-main"; Path = $path }) | Out-Null

            if ($theme -eq "Midnight") {
                $densePath = Join-Path $outputDir "workflow-dense-graph.png"
                Capture-Window -Handle $session.Handle -Path $densePath -CaptureWidth 1280 -CaptureHeight 820
                $captures.Add([pscustomobject]@{ Name = "workflow-dense-graph"; Path = $densePath }) | Out-Null

                Stop-QaApp -Process $session.Process
                $session = $null

                $stagePath = Join-Path $outputDir "workflow-stage-mode-before-switch.png"
                $session = Start-QaApp -Theme $theme -Arguments @("--visual-qa-stage")
                Capture-Window -Handle $session.Handle -Path $stagePath -CaptureWidth 1280 -CaptureHeight 820
                $captures.Add([pscustomobject]@{ Name = "workflow-stage-mode-before-switch"; Path = $stagePath }) | Out-Null

                Stop-QaApp -Process $session.Process
                $session = $null

                $stageAfterSwitchPath = Join-Path $outputDir "workflow-stage-mode-after-patch-next.png"
                $session = Start-QaApp -Theme $theme -Arguments @("--visual-qa-next-patch", "--visual-qa-stage")
                Capture-Window -Handle $session.Handle -Path $stageAfterSwitchPath -CaptureWidth 1280 -CaptureHeight 820
                $captures.Add([pscustomobject]@{ Name = "workflow-stage-mode-after-patch-next"; Path = $stageAfterSwitchPath }) | Out-Null

                Stop-QaApp -Process $session.Process
                $session = $null

                $mainAfterSwitchPath = Join-Path $outputDir "workflow-main-after-patch-next.png"
                $session = Start-QaApp -Theme $theme -Arguments @("--visual-qa-next-patch")
                Capture-Window -Handle $session.Handle -Path $mainAfterSwitchPath -CaptureWidth 1280 -CaptureHeight 820
                $captures.Add([pscustomobject]@{ Name = "workflow-main-after-patch-next"; Path = $mainAfterSwitchPath }) | Out-Null
            }
        }
        finally {
            if ($session) {
                Stop-QaApp -Process $session.Process
            }
        }
    }

    $dialogSpecs = @(
        @{ Name = "plugin-search"; Title = "Add Plugin"; Arguments = @("--visual-qa-plugin-search") },
        @{ Name = "preferences"; Title = "Misc Settings"; Arguments = @("--visual-qa-preferences") },
        @{ Name = "nam-browser"; Title = "NAM Model Browser"; Arguments = @("--visual-qa-nam-browser") },
        @{ Name = "ir-browser"; Title = "IR Browser"; Arguments = @("--visual-qa-ir-browser") }
    )

    foreach ($theme in $themes) {
        foreach ($dialog in $dialogSpecs) {
            $session = $null
            try {
                $session = Start-QaApp -Theme $theme -Arguments $dialog.Arguments
                if ($null -eq $dpi) {
                    try { $dpi = [VisualQaWin32]::GetDpiForWindow($session.Handle) } catch { $dpi = 96 }
                    Assert-ExpectedScale -ActualScalePercent ([Math]::Round(($dpi / 96.0) * 100))
                }

                $safeTheme = $theme.ToLowerInvariant().Replace(" ", "-")
                $dialogHandle = Find-QaWindow -Process $session.Process -Title $dialog.Title
                $path = Join-Path $outputDir ("dialog-{0}-{1}.png" -f $dialog.Name, $safeTheme)
                Capture-Window -Handle $dialogHandle -Path $path
                $captures.Add([pscustomobject]@{ Name = "dialog-$($dialog.Name)-$safeTheme"; Path = $path }) | Out-Null
            }
            finally {
                if ($session) {
                    Stop-QaApp -Process $session.Process
                }
            }
        }
    }
}
finally {
    if ($settingsExisted) {
        Copy-Item -LiteralPath $settingsBackup -Destination $settingsPath -Force
    }
    elseif (Test-Path $settingsPath) {
        Remove-Item -LiteralPath $settingsPath -Force
    }

    if ($defaultPatchExisted) {
        Copy-Item -LiteralPath $defaultPatchBackup -Destination $defaultPatchPath -Force
    }
    elseif (Test-Path $defaultPatchPath) {
        Remove-Item -LiteralPath $defaultPatchPath -Force
    }
}

$scalePercent = if ($dpi) { [Math]::Round(($dpi / 96.0) * 100) } else { "unknown" }
$summary = [pscustomobject]@{
    app = $appPath
    patch = $qaPatch
    output = $outputDir
    dpi = $dpi
    scalePercent = $scalePercent
    expectedScalePercent = if ($ExpectedScalePercent -gt 0) { $ExpectedScalePercent } else { $null }
    captures = $captures
}

$summary | ConvertTo-Json -Depth 5 | Set-Content -Path (Join-Path $outputDir "capture-summary.json") -Encoding UTF8
$summary
