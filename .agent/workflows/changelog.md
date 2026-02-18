---
description: Update CHANGELOG.md after completing work
---

# Changelog Update Workflow

Update the project's CHANGELOG.md following [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) format.

## Keep a Changelog Format

```markdown
# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- New features

### Changed
- Changes to existing functionality

### Deprecated
- Soon-to-be removed features

### Removed
- Removed features

### Fixed
- Bug fixes

### Security
- Security fixes

## [1.0.0] - YYYY-MM-DD

### Added
- Initial release features
```

## Steps

1. **Locate or create CHANGELOG.md** in the project root using the format above

2. **Add entries to `[Unreleased]`** under the appropriate category:
   - `Added` – New features
   - `Changed` – Changes to existing functionality  
   - `Deprecated` – Soon-to-be removed features
   - `Removed` – Removed features
   - `Fixed` – Bug fixes
   - `Security` – Vulnerability fixes

3. **Entry format:**
   - Start with **bold component name** if applicable
   - Keep entries concise but descriptive
   - Use past tense ("Added", "Fixed", not "Add", "Fix")

4. **For releases:** Move `[Unreleased]` content to new version section with date

## Guiding Principles (from keepachangelog.com)

- Changelogs are for **humans**, not machines
- Every version should have an entry
- Group by type of change
- Versions should be linkable
- Latest version comes first
- Release date for each version
- Mention if you follow Semantic Versioning
