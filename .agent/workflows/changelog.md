---
description: Update CHANGELOG.md after completing work
---

# Changelog Update Workflow

After completing a feature, fix, or significant change, update the project's CHANGELOG.md.

## Steps

1. **Locate or create CHANGELOG.md** in the project root
   - If none exists, create one following [Keep a Changelog](https://keepachangelog.com/) format

2. **Find the unreleased/development section** at the top of the changelog
   - Common names: `[Unreleased]`, `[Development]`, `[Next]`, or the current dev version like `[X.Y.Z-dev]`
   - If no such section exists, create one below the header

3. **Add entry under the appropriate category:**
   - `### Added` – New features or capabilities
   - `### Changed` – Changes to existing functionality
   - `### Fixed` – Bug fixes
   - `### Removed` – Removed features
   - `### Deprecated` – Soon-to-be removed features
   - `### Security` – Security fixes

4. **Write a concise entry:**
   - Start with **bold feature/component name** if applicable
   - Include brief description of what changed
   - For bug fixes, optionally add root cause synopsis

5. **For significant bug fixes**, consider adding to a Technical Fixes Reference section with:
   - Symptom
   - Root cause
   - Solution summary
   - Files changed

## Example Entry

```markdown
### Fixed
- **VSTi Audio Output Pins** – Root cause: wrapper class hid real plugin's bus state. Solution: unwrap before querying buses.
```
