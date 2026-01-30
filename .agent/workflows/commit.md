---
description: Commit changes with proper message and changelog update
---

# Commit Workflow

Commit completed work with a well-structured message and changelog update.

## Steps

1. **Check git status** to see modified files
   ```
   git status
   ```

2. **Stage relevant files** (not build artifacts or temp files)
   ```
   git add <files>
   ```

3. **Update CHANGELOG.md** before committing
   - Follow `/changelog` workflow
   - Stage the changelog with the other changes

4. **Write commit message** following Conventional Commits:
   - `feat:` – New feature
   - `fix:` – Bug fix
   - `docs:` – Documentation only
   - `refactor:` – Code change that neither fixes nor adds
   - `perf:` – Performance improvement
   - `test:` – Adding/updating tests
   - `chore:` – Maintenance tasks

5. **Commit format:**
   ```
   <type>: <short summary>

   <optional body with details>
   
   <optional footer with breaking changes, issue refs>
   ```

6. **Commit the changes**
   ```
   git commit -m "<message>"
   ```

## Example

```bash
git add src/PluginComponent.cpp src/BypassableInstance.h CHANGELOG.md
git commit -m "fix: VSTi audio output pins not displaying

Root cause: BypassableInstance wrapper hid plugin's bus state.
Solution: Unwrap wrapper before querying buses.

Files: PluginComponent.cpp, BypassableInstance.h, FilterGraph.cpp"
```

## Quick Reference

| Change Type | Prefix | Changelog Section |
|-------------|--------|-------------------|
| New feature | `feat:` | Added |
| Bug fix | `fix:` | Fixed |
| Breaking change | `feat!:` or `fix!:` | Changed |
| Deprecation | `refactor:` | Deprecated |
| Removal | `refactor:` | Removed |
