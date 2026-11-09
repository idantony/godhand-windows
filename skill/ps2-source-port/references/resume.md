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
- User authorized public GitHub idantony/godhand-windows. Latest cadence: small
  local commits retain rollback points; push only substantial execution milestones.
  After every code commit, review the whole project's execution frontier,
  blockers and critical path before choosing the next steps and assignments.
  After milestone pushes, provide a subjective workload estimate in chat only;
  do not record or publish that percentage in the repository.
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
has nine recovered allocation/release/init/move/lifecycle functions. Its actual
constructor count and destructor list are empty; this specialization is not a
generic CRT. Native memory allocation and three task-manager functions are also
implemented. The integrated build includes six modules and a C++ linkage test.
Consult current source/reports before assuming
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

Next concrete slices: implement actual task step/thread/start/wait semantics and
the remaining subsystem initialization, then integrate real boot platform
operations. Original cHeap initial capacity/fragmentation behavior still requires
evidence; the current memory backend supplies explicit host policies only.
Continue toward renderer/audio/input and a first real scene. The nine r207
functions, three task helpers and host memory adapter are already implemented:
do not restart them. Coordinate file ownership
with actual current workers when parallel work is running.
Record unsupported instructions, inferred names and unconnected backends.

GPU use was explicitly authorized for needed work. Actual host capability
records remain local in analysis/gpu-host.json and analysis/gpu-capabilities.json;
do not infer GPU memory from the overflowing legacy CIM AdapterRAM field. GPU
availability does not establish that any game renderer has been ported.

The public GitHub first verified milestone is commit
`a7a72e3d13e21402482bc2b552491f30a908e246`. Check the actual local/remote heads before
new pushes; do not assume this recorded commit is the newest. Only curated
`docs/progress.json` is published, not raw local `analysis/` evidence.

Maintain this resume as meaningful work finishes, linking durable reports rather
than copying large generated material. No need to wait for the full port before
updating the skill: the user explicitly authorized earlier maintenance.
