# Active project resume: God Hand

Snapshot from the 2026-11-10 session. Read current git status, instructions and
reports before claiming passes; working agents may have newer uncommitted files.
This skill is maintained during development, not only after the full port.

## Scope and authority

Recover readable original-game source and make a standalone native Windows port,
inspired by OpenGOAL. No gameplay remake, packaged emulator or generic translated
runtime as the final product. No playable scene has been observed yet.

- Useful source/tools/private evidence: E:/godhand_windows.
- Builds, cache, temporary downloads, headless projects and large logs:
  G:/middle_thing/godhand_windows. Do not clean other projects or unknown files.
- Tools may be installed without asking again for this project. GPU use and
  parallel delegation are authorized. Development agents at most gpt-6.1-sol;
  prefer gpt-6-luna for bounded mechanical checks. No Codex Fast.
- Public repository authorized: https://github.com/idantony/godhand-windows.
  Small LOCAL commits preserve rollback; push substantial milestones only.
  Review actual execution frontier/critical path after each code commit.
  After milestone pushes, give subjective workload percentage in chat only;
  never put it in GitHub or confuse it with upstream matching coverage.
- Public README is bilingual. Publish authored source/tests/curated summaries;
  exclude ISO/ELF/assets, raw decompiler/disassembly, SDKs, dependencies and keys.
- Maintain this independent skill folder. Canonical path is
  E:/godhand_windows/skill/ps2-source-port; discovery junction is
  C:/Users/sutong/.codex/skills/ps2-source-port.
- None of this authority transfers to another game or a read-only request.

## Baseline and source

ISO: God Hand (USA)(Uncensor Patch v1.0).iso, SLUS-21503, NTSC-U1.00.
The actual patched ELF and loaded image differ from upstream retail; original
local bytes are ground truth. All81 SNR2 overlay hashes match upstream manifest.
Pin vendor/god-hand-decomp at eaae18d0dfab1f9a0df276ae5ca1581ad326faf3.
Read references/godhand-case.md for exact hashes and rejected assumptions.

## Current execution frontier

The native developer host begins at task0030D358 in original slot3 of a four-slot
manager. It connects real sound-bank commits, core/language DVD reads and fiber
waits, font/MST/MSA binding, menu MSD tables, global ID binding and IDD/TBL reset.
Default USA run executes002AE8A8's recovered native MC prefix and reaches
mcport.rel Control+02E8 after eight scheduler visits, with67 real UI entries
and packed message state live. CLI exits2 for that explicit frontier, never
game success. Module SHA256 binding rejects mismatched source versions.
Earlier boot/GS/IOP initialization and the full main loop have NOT executed.
The original negativeflags598 branch now registers real GameMain and the
parallel asset loader. Actual no-draw gates and all eight case0 initializers run,
then stop at state1 handler002A5BB8. Asset loading drains before that frontier:
current measured run95 scheduler visits, game_state1,rate0.5,assets loaded.
This explicitly selected original branch does not replace default MC startup.

002AE8A8 loads mcport.rel and calls imported Main__7cMcBoot. Literal jal0 there
is a dynamic relocation, not a no-op. Full memory-card Main depends on actual ID
UI/save behavior still under recovery. Native GetInfo/SearchFile and recovered
slot/file-check slice exist separately; the native module/Main/DispInit prefix
is now connected to the developer host, not complete Main/Control.
Actual67-entry IDBase initialize/build/link/reset, typed640-entry/100-job pools,
dummy-entry/family operations and packed-message binding/environment now pass
separately. The MC wrapper now calls these actual operations. Control and
actual final message/sprite submission are still being connected;
do not restart those completed primitives or treat them as full UI execution.

Task fibers preserve serial start/sleep/exit and same-slot tail restart. Restart
success does not return to the old stack; owner deletes it and next selected
visit starts a new fiber with state2. State4/external EE wakeups, arbitrary kernel
syscalls, interrupt and priority preemption remain unsupported.

Other implemented units: two RNG functions, boot0030B298 callback/control slice,
nine r207 routines, real bounded host heap, task selection/inheritance/traversal,
DVD jobs with actual sector padding/priority/pressure drain/extra WaitFrame sleep,
core bundles, restricted TIM2 RGBA, SCR mesh descriptors and SES/ADX headers.
New30D5C8 asset loader reads original41-object table/core/player resources; it is
separately verified, not yet reached through complete original startup.

Font texture primitive decodes actual CT32 upload storage into PSMT4 sampling;
paired codes share cells and choose two CLUT bases by parity. Original glyph1
is a26x32 triangle button. Final screen transforms/tint/blend/message consumers
are separate work. Offscreen RTX upload/readback is asset QA, not game rendering.
The new optional tools/build-ui-qa.ps1 separately builds native D3D11 font
fragments. It renders an actual original quad using verified GS modulation,
alpha-test and blend equations;12288 GPU pixels match the limited CPU equation.
Only integer edges/1:1 nearest and explicitly supplied global GS context are
supported. This is a real restricted draw backend, not a game scene/window.
Sound startup owns core/pl00 program/sample tables and AFD→AFS directory maps,
plus BGM0 tables. Ready7f/7f/0f reflects those actual commits; voices, event
execution, encrypted ADX decode and audible output are still absent.

## Evidence map (private analysis/)

| Question | Reports |
|---|---|
| Identity/pin/tools | disc-baseline.json, patched-baseline-comparison.json, upstream-audit.json, rea-status.json, toolchain-manifest.json |
| Startup | ghidra-startup.json, boot-port-evidence.json, startup-task-evidence.json, startup-host-verification.json |
| Native tests | native-verification.json (current module list, actual test steps, source fingerprints) |
| RNG | fpu-rounding-evidence.json, rng-differential.json |
| Tasks/DVD | task-thread-semantics.json, task-replace-evidence.json, dvd-job-semantics.json |
| Font/menu | font-state-evidence.json, font-texture-evidence.json, menu-state-evidence.json |
| Audio/MC/game assets | audio-startup-semantics.json, memory-card-semantics.json, game-start-evidence.json |
| IDBase, messages and GameMain case0 | id-base-semantics.json, packed-message-evidence.json, game-init-evidence.json |
| ConnectedMC prefix and ID consumers | mc-task-semantics.json, id-jobs-evidence.json |
| GPU QA | gpu-capabilities.json, gpu-texture-verification.json, gpu-qa-last-run.json |
| Restricted font fragment GPU draw | ui-renderer-verification.json, font-draw-evidence.json |
| Reproducibility | fresh-build-verification.json (older scope; compare its source/test list before reuse) |

RNG differential:10272 seeds/20544 exact comparisons against a restricted model
of actual instructions; not console traces. File existence is not a pass. The
public docs/progress.json is curated, not a substitute for private evidence.

## Reproduce / continue

```powershell
# Working directory E:/godhand_windows
python -B tools/build-native.py
python -B tools/verify_rng.py
& 'G:/middle_thing/godhand_windows/build/native/run-startup.exe' '<own ISO>'
python -B tools/export-progress.py
python -B tools/check-public-tree.py
```

Compiler: .tools/zig-x86_64-windows-0.17.1/zig.exe cc. Build accepts --compiler
and --scratch for independent source-copy checks. Use -B to avoid pycache.
Input preparation: tools/prepare-inputs.py --iso '<own ISO>' pins official vendor
and creates bounded references; never runs upstream private SDK setup.
Standalone Ghidra12.1.4+EE Reloaded/JDK21 analyzes R5900. REA6.8.0 Windows gate
does not accept this PS2 ELF; don't misclassify it. tools/run-ghidra.ps1 keeps
headless projects onG. REA was checked current at the session's audit time.

Critical work: full MC initialization/UI and original continuation, actual
GameMain preframe/state0 dependencies, font screen drawing, earlier boot and
full loop integration, then native scene renderer/model/animation/audio/input.
Use current worker ownership before edits; do not restart completed slices.
After every commit reassess which original call can execute next; prioritize
connected consumers over more isolated metadata parsers. Check git heads before
pushing; recorded historical commits are not assumptions about remote state.

The user explicitly requested continuing until fully complete while they sleep.
A persistent goal is active for the full native port; do not mark complete at
a module/milestone or pause merely because a commit/push finished. No token
budget was requested. Current chat-only workload estimate after pushedae62976
is stored privately in analysis/project-review.json, never public progress.
