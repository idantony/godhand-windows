# Active project resume: God Hand

2026-11-10 milestone snapshot. Reconcile Git, live worker ownership and source
fingerprints before claiming current passes; workers may have newer changes.
This skill is maintained during development, not only after the complete port.

## Scope and authority

Recover readable original-game source and make a standalone native Windows port
following OpenGOAL's method. No gameplay remake, packaged emulator or generic
instruction-translated final runtime. No playable original scene verified yet.

- Useful files: E:/godhand_windows. Intermediate builds/cache/headless projects/
  large logs: G:/middle_thing/godhand_windows. Do not clean unknown projects.
- Needed tool installation, GPU and parallel agents are authorized. Development
  agents at most gpt-6.1-sol, mechanical tasks prefer gpt-6-luna. No Codex Fast.
- Public repo https://github.com/idantony/godhand-windows. Local commits for
  rollback; push major milestones only. Review execution frontier after every
  code commit. After pushes, give rough workload percentage in chat/private
  analysis only, never GitHub or as upstream matching coverage.
- README English/Chinese. Publish authored source/tests/curated documentation;
  exclude game data, raw disassembly/decompiler dumps, tools/dependencies/keys.
- Independent canonical skill E:/godhand_windows/skill/ps2-source-port, discovery
  junction C:/Users/sutong/.codex/skills/ps2-source-port.
- User explicitly says continue until fully complete while they sleep. A
  persistent active goal has no requested token budget. Do not mark it complete
  at a module/push, pause without instruction, or stop after this resume.
- These historical permissions do not transfer to another game/read-only task.

## Baseline

God Hand (USA)(Uncensor Patch v1.0).iso, SLUS-21503, NTSC-U1.00. The patched
ELF/load image differ from retail. All81 overlays match upstream manifest.
Vendor pin eaae18d0dfab1f9a0df276ae5ca1581ad326faf3. Exact hashes, numerical,
global-identity, graphics and relocation corrections: [godhand-case.md](godhand-case.md).

REA6.8.0 was current at audit, but its Windows gate excludes this PS2 ELF.
Standalone Ghidra12.1.4+EE Reloaded2.1.38/JDK21 handles R5900. Tools are local.
Zig0.17.1, Windows SDK10.0.26100.0 x64, FFmpeg9.0.2 essentials are installed.
Retain dependency licenses and verify pinned archives; no private SDK execution.

## Latest verified connected execution

- Developer native host begins at original0030D358 in Main4task slot3. Earlier
  boot/GS/IOP/full frame loop have NOT run. Real banks, DVD reads/fiber waits,
  font/menu/ID initialization connect to002AE8A8 and true relocated MC Main.
- Default host prefix mode preserves the bounded first-frame check: without
  draw consumer, true002AB0B8 frontier at tick8; with real consumers it draws one
  frame and reports mcport+2E8 at tick9. Not whole-task/game completion.
- Explicit continuation mode runs real MC Control, actual67-entry UI text/window/
  TIM2cursor, native digital mapping, limited genuine SFX, normal mc_exit, same
  slot replace_current0030DA08, Movie0 and original Main/asset-loader tasks.
- Root's hidden/muted Windows QA target presents6 MC frames and90 Clover frames,
  waits actual videoEOF/audioDONE, then reaches game_state3 with assets loaded,
  real r020 room data/RHT pools, frontier002BE708.195 timed scheduler visits in
  that run. Keys are a labeled synthetic fixture through actual ELF table1.
  No human control or playable original scene is claimed. Cleanup succeeded.
- A common MC renderer uses original draw order/palette, separate nearest font
  and four-bit bilinear sprite GPU pipelines, explicit GS context. First frame
  rawSHA256 remains6bd058ff0236fd6f2e7cf9fb201c8c394c456f97bf37b3ff8ddd702d9c5a0cf8.
 32x32 PSMT4 TIM2 cursor and prior T32 window each pass229376 independent CPU/GPU
  pixel checks. Full visible absent/empty-profile flow separately passes.
- Native Movie0 uses bounded direct ISO subfile/FFmpeg,640x448@30000/1001,90
  frames and123328 decoded PCM sampleframes; owned queues actually complete.
  Manager/three tracks share one canonical movie_pool, not a second cold state.
- Real MC voice output uses XAudio2, actual descriptor-chain program/sample
  binding, original queue/priority/quantized pitch, default level90. DSP is an
  explicit native reference, not SPU2 Gaussian/reverb equivalence. Actual2CA470
  unconditionally calls2CA5D8 each audio update; later evidence corrected an
  earlier suggestion to flush only conditionally. Other audio callees remain.
- GameMain0/1/2 and case3 prefix share real save/init camera/stage/children/HUD/
  room state. Cockpit39runs405entries, original object slots and RHT arenas exist.
  Room20 is the actual title module, not empty r207. No model/animation/combat
  rendering or complete save/read path is claimed.
- Scenario has16 records distinct from Main4; explicit run_nested preserves
  root/caller fibers. Release Scenario/Cockpit before root backend/ID/assets.

Foundation checkpoint bc9a6f4 was built from independent staged tree
cec6256f927277399d5280550b90758d6d86a693:55 C modules/56 checks, including muted
hardware/codec checks. The later integrated working snapshot also passed55/56.
Read current HEAD/remote; do not assume historical milestones are latest code.
RNG:10272 seeds/20544 exact modeled comparisons; not hardware captures.

## Immediate ownership and next work

- Root: native Windows app, reusable MC renderer, build/verification/review,
  checkpoint docs/push. Production native target only accepts actual input;
  separate startup-QA target compiles synthetic confirmation support.
- skill_forward_test: native R020 binding/title logic, new borrowed services
  scene view. Preserve actual main-module relocation/symbol identity.
- godhand_upstream: new case3 tail after overlay binding and next actual scene
  handlers, borrowed canonical services owners. Do not duplicate Scenario/pools.
- opengoal_method: actual R020 logo Fade.start/status and native flat overlay
  backend. No observed PlayMovie call in title text; do not invent title FMV.

Check live owners before edits. They may add new files/modify services after the
verified checkpoint. New work must not be swept into an untested staged commit.
R020 Init/Move are real main imports: verified room-record+8/+C, not+4. Relocation
symbol index uses+5u16 and addend+8u32; old inventory unpack is unreliable. The
configured Release FST token has proven first-child alias semantics. Do not run
guest code or implement a broad missing-path fallback to hide these dependencies.

Largest remaining work: title/room native callbacks and scene consumers, model/
animation/object logic, full frame/boot, combat/audio/input, scene transitions,
save/load and full progression acceptance. Prefer next connected original call
over more isolated parsers. A native window/clip is not a completed game port.

## Evidence and reproduction

Private analysis/ reports:

- native-verification.json: current C module checks and source fingerprints.
- native-core55-snapshot-verification.json: independent bc9a6f4 foundation scope.
- windows-{native,startup-qa,mc-frame}-build.json: source/executable hashes.
- native-startup-window-verification.json: real connected HWND execution proof.
- mc-prefix-reusable-verified.json and mc-prompts-render-evidence.json: true UI.
- ui-sprite-verification.json, window-host-evidence.json: scoped GPU/window QA.
- movie-task-evidence.json, movie0-native-last-run.json: codec/device/frame proof.
- audio-events/se-voice/sound-control-semantics.json: limited actual audio.
- game-*-evidence.json: actual GameMain/stage/room/owner boundaries.
- disc-baseline.json, patched-baseline-comparison.json, upstream-audit.json,
  rea-status.json, toolchain-manifest.json: input/tools and pin.
- project-review.json: private commit reviews/rough workload estimate.

```powershell
# E:/godhand_windows; user-owned ISO/codec paths explicit
python -B tools/build-native.py --hardware-audio --movie-codec '<ffmpeg.exe>'
python -B tools/verify_rng.py
python -B tools/build-windows.py --target native
python -B tools/build-windows.py --target startup-qa
# See README for hidden/muted bounded QA execution and frontier exit4.
python -B tools/export-progress.py
python -B tools/check-public-tree.py
```

Export rejects stale module/window fingerprints or replaced execution binaries.
Private evidence paths are not public assets. File existence is not a pass.
Build scripts keep outputs/cache/logs onG and accept compiler/scratch overrides.
Skill validation: python -B C:/Users/sutong/.codex/skills/.system/skill-creator/scripts/quick_validate.py
E:/godhand_windows/skill/ps2-source-port (one command). Never serialize credentials.
