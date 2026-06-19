# NeuralAmpModelerCore A2 Vendor Notes

Upstream repository: https://github.com/sdatkinson/NeuralAmpModelerCore

Version: `v0.5.3`

Commit: `9c7b185de346fe0725dea537bcee4bc38b5bb6d6`

License: MIT, preserved in `LICENSE`.

Copied files:

- `NAM/**`
- `LICENSE`

Build dependency note: upstream NAM includes `json.hpp`; Pedalboard3 resolves
that include through the existing CPM `nlohmann_json` dependency instead of
copying upstream's vendored single-header JSON file.

Reuse mode: direct-copy for an isolated C++20 A2 compile island. The existing
Pedalboard3 `NAMCore` runtime path still uses `external/NeuralAmpModelerCore`
until the A2 adapter slice has explicit A1/A2 coverage.
