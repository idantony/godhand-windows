# Active project resume: God Hand

Snapshot from the 2026-11-10 session; reconcile it with current files and reports
before new completion claims. The skill may be edited during ongoing work.

## User scope and authorization

Requested outcome: readable original-game source recovery plus a standalone
Windows port inspired by OpenGOAL's method, not a gameplay remake or emulator
wrapper. User authorized continuing parallel agent work, required tool
installation and immediate ongoing skill maintenance.

For this project only:

- Durable source, useful analysis and project tools live under E:/godhand_windows.
- Builds, downloads in progress, caches, temporary files and large logs live
  under G:/middle_thing/godhand_windows in dedicated subdirectories. Never clean
  other projects. Do not relocate existing unknown material.
- Development-agent models are at most gpt-6.1-sol; prefer gpt-6-luna for narrow
  mechanical work. No gpt-6-sol, gpt-6-astra or Codex Fast. Do not imply historical
  logs prove active sessions were migrated.
- User authorized progress pushes to public GitHub idantony/godhand-windows.
  Review diffs/ignore rules; publish distributable authored code/instructions
  and non-game-data reports only. Exclude ISO/ELF/assets/SDK/tool binaries and
  credentials. This is not publishing permission for future projects.

Canonical skill: E:/godhand_windows/skill/ps2-source-port.
Discovery junction: C:/Users/sutong/.codex/skills/ps2-source-port.
Automatic discovery stays enabled by default.

## Reports to read as needed

All following paths are relative to E:/godhand_windows:

| Question | Report |
| --- | --- |
| Disc and actual patch identity | analysis/disc-baseline.json, analysis/patched-baseline-comparison.json |
| Upstream recovery/licensing | analysis/upstream-audit.json |
| Assets and overlays | analysis/asset-index.json, analysis/overlay-index.json, analysis/asset-parser-verification.json |
| Local startup analysis | analysis/ghidra-startup.json |
| RNG semantics/differential | analysis/fpu-rounding-evidence.json, analysis/rng-differential.json |
| Integrated native checks | analysis/native-verification.json |
| Boot control flow/platform boundary | analysis/boot-port-evidence.json |
| First overlay investigation | analysis/r207-source-evidence.json |
| Tools | analysis/toolchain-manifest.json, analysis/ghidra-install.json, analysis/rea-status.json |

Native RNG and read-only asset layers passed recorded tests. RNG report records
10272 seed inputs and 20544 comparisons against a restricted original-instruction
model, not console traces. src/core/boot.{h,c} compiles and its callback/order/
layout tests pass; actual platform callbacks are not connected. The r207 slice
has six recovered allocation/release/init/move functions. Three CRT/once lifecycle
helpers remain pending in that snapshot. The integrated build includes all four
modules and a C++ linkage test. Consult current source/reports before assuming
the snapshot is exhaustive; do not infer active workers from this document.

The project is not playable. These reports do not prove real-scene execution,
graphics/audio/input integration, complete overlays or save/load behavior.
A DLL, parser and callback test harness are limited validated modules.

## Reproducible entrypoints

Working directory E:/godhand_windows. Inspect scripts/options before execution.

```powershell
python -B ./tools/build-native.py
python -B ./tools/verify_rng.py
python -B ./tools/inspect_disc.py './God Hand (USA)(Uncensor Patch v1.0).iso'
python -B ./tools/inspect_assets.py './God Hand (USA)(Uncensor Patch v1.0).iso'
./tools/run-ghidra.ps1
```

Existing project compiler is .tools/zig-x86_64-windows-0.17.1/zig.exe cc; use
project build scripts to keep output/cache on G. Do not download private Sony
SDKs or execute upstream setup merely to advance matching. Existing REA settings
are unrelated environment state; use this project's separate PS2 Ghidra backend
without changing another project's configuration.

Next concrete slices: finish r207 CRT/once lifecycle from actual table and relocation
evidence, connect a real native memory adapter to its allocation boundary, then
integrate task/startup dependencies and real boot platform operations. Continue
toward renderer/audio/input and a first real scene. The six existing r207
functions are already recovered: do not restart them. Coordinate file ownership
with actual current workers when parallel work is running.
Record unsupported instructions, inferred names and unconnected backends.

The public GitHub first verified milestone is commit
`a7a72e3d13e21402482bc2b552491f30a908e246`. Check the actual local/remote heads before
new pushes; do not assume this recorded commit is the newest. Only curated
`docs/progress.json` is published, not raw local `analysis/` evidence.

Maintain this resume as meaningful work finishes, linking durable reports rather
than copying large generated material. No need to wait for the full port before
updating the skill: the user explicitly authorized earlier maintenance.
