---
description: Archive build artifacts before clean rebuild
---

# Rebuild Workflow

Archive current build before deleting build directory or doing a clean rebuild.

## Steps

// turbo-all

1. **Check if build artifacts exist**
   - Look for build output directory (e.g., `build/`, `out/`, `bin/`)
   - Identify the main executable or library targets

2. **Archive current build** (if artifacts exist)
   ```powershell
   # Create releases folder if it doesn't exist
   if (!(Test-Path "releases")) { mkdir releases }
   
   # Create timestamped archive
   $timestamp = Get-Date -Format "yyyy-MM-dd_HHmmss"
   $archiveName = "releases/build_$timestamp"
   
   # Copy key artifacts
   mkdir $archiveName
   Copy-Item "build/*_artefacts/Release/*.exe" $archiveName -ErrorAction SilentlyContinue
   Copy-Item "build/*_artefacts/Release/*.dll" $archiveName -ErrorAction SilentlyContinue
   Copy-Item "build/*_artefacts/Release/*.vst3" $archiveName -Recurse -ErrorAction SilentlyContinue
   ```

3. **Tag in git** (optional but recommended)
   ```bash
   git tag -a "pre-rebuild-$(date +%Y%m%d)" -m "Build snapshot before clean rebuild"
   ```

4. **Delete build directory**
   ```powershell
   Remove-Item -Recurse -Force build
   ```

5. **Reconfigure and rebuild**
   ```powershell
   cmake --preset default
   cmake --build build --config Release
   ```

## Quick Archive Command (PowerShell)

```powershell
$ts = Get-Date -Format "yyyyMMdd_HHmm"
if (!(Test-Path releases)) { mkdir releases }
Copy-Item build/*_artefacts/Release/*.exe "releases/build_$ts.exe" -ErrorAction SilentlyContinue
```

## Why Archive?

- **Compare behavior**: Test if a bug existed in older builds
- **Emergency rollback**: Ship old build if new one has critical issues
- **Documentation**: Prove what was working at a point in time
