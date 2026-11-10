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

## Latest verified title checkpoint (2026-11-10)

- Immutable source: G:/middle_thing/godhand_windows/snapshots/title-native-04.
  Build/cache/output: G:/middle_thing/godhand_windows/verification/title-native-04.
  105 C modules/111 checks PASS, all hardware/codec/full ADX opt-ins, no skips.
  Fresh DLL RNG checks:20544/20534 exact comparisons respectively.
- Matching Windows startup-QA:600ticks,6MC/90Movie/481Title frames, state4,
  no frontier/error. Original logos advance by timers; actual title remains at
  PRESS START. Private capture reference/render/native-title-verified04-frame350.
  Earlier boot/world scene/combat/animation/save are NOT verified.
- Current analysis/native-verification.json and window proof refer to THIS
  copied source, not whatever workers now edit. Use export-progress.py
  --verified-source-root with the snapshot; stage exact verified source blobs
  from its source fingerprints, leaving later working edits intact.
- tools/freeze-source.py copies authored files consistently into NEW G: folders;
  read-only-used .tools/reference junctions borrow E: inputs. Never modify a
  finished copy. Build with --scratch outside that source directory. The native
  runner compiles independent modules/tests with4 compiler workers; device
  tests remain serial. Source guard detects in-run changes.
- Root host owns scene frame lifecycle BEFORE all GlobalTask traversal. Loading
  SystemMenu can run before Main; beginning only inside state5 erases its jobs.
  Resource views expire each begin; completed pixels, not borrowed ID jobs, are
  presented on the next update. TitleEnd must preserve SystemMenu resource1.
- First no-frontend frontier is now15B880. Immediate host stop precedes the
  independent asset fiber's completion; tests must not require its old
  >90tick/assets-loaded side effect. Full Windows execution verifies assets.
- Root wired state5/SoundExit/SystemMenu owners; the verified integrated
  NewGame boundary was2CB8A0 bank2 sound. Audio worker subsequently FROZE real
  bank2 programs0/10/11/12, Stop10 integer ramps and source gains; these newer
  bytes require a NEW fresh build, cannot inherit title-native-04's proof.
- Workers: opengoal_method owns new game_scene_meshes/canonical pool handles;
  godhand_upstream owns native static-mesh streams/transform research;
  skill_forward_test owns exact scene-exit cleanup and next-room lifecycle.
  Root owns host/services/Main integration, GPU backend and coherent proof.

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

Later local checkpoint:64 C modules/66 checks pass. Connected Windows run still
presents6 MC/90 movie frames in195 ticks but advances through actual R020 module
identity loading, room SE bank2, collision/effects/filter setup to002D6F40.
Known outer-frame leaves now use observed31CEB8 seed, shared two-MT advance and
proper Movie-before-tasks/Fade-after-tasks order. Finite frame RNG adds20534
actual-instruction comparisons over10267 words. See frame-rng-differential.json.
R020 Init/Move are still not reached in this checkpoint; later worker changes
may advance that frontier. Public pushed milestone4578b62 precedes this local
checkpoint. Read Git rather than assuming the snapshot is current.

The real title encrypted ADX decoder is now assigned to opengoal_method. Primary
VGMStream support was found, actual key string/callsite identified in the user's
ELF, and the full6369200 stereo frames match independent vgmstream PCM in the
worker's initial test. This is decoder evidence, not playback or BGM Init success;
real stereo queue/provider integration is still ongoing. Codec source/tests are
not part of the above64-module checkpoint until explicitly built/integrated.

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


New local checkpoint after b2f5708:84 C modules/85 checks passed with real muted
hardware, FFmpeg and independent vgmstream WAV. Windows195ticks/6MC/90Movie now
reaches002BF8F8, not2D6F40. Native ADX/BGM/LogoSE and new scene leaves are built.
Direct title-ui target renders the original warning screen at Move4 using actual
R020 Init/Move; explicit black512448 clear and final OT order remain scoped QA,
not the whole scene. Raw bf70bcf724b7b776ddf3e4faf436aef137bbe5e44c108cf00b3be26ab0cd8ebb.

Current workers are adding full case3 tail/Title binding (skill_forward_test),
state4 order and final OT audit (opengoal_method), late camera using the same
canonical ACD owner (godhand_upstream). Root integrates, verifies and owns the
canonical cEvent prefix initialized from actual mapped ELF/BSS before state4;
services must borrow it. New game_scene_step is not in the above84-module suite.
Preserve explicit path-distance and title-player allocation memory profiles.
Last pushed milestone is still4578b62; subjective percentage belongs only in
chat/private review and was8. Keep working until the full playable port goal.


Latest committed local checkpoint is b25aa28 (not pushed). Source after it is
in progress. Root added a canonical display_state module (ELF scalars and real
2A48A8 static leaf, isolated test passed), semantic room registry views, and a
bounded renderer UI group queue (strict compile only, not GPU-verified yet).
The latter captures entry/base scalars and borrows resource/font/menu owners
until same-frame flush; a full permutation is required. Original OT ordering
worker will provide indices. Existing immediate path remains the old reference.

Important new primary OT finding: actual14FF00 head-inserts groups (LIFO),
31A650 bucket=count-min(depth,count), bucket traversal ascending. Supported MC
textures use bank6/depth5; an entire text message is ONE bank6/depth2 group
(keep glyph order inside it). Therefore final original OT draws windows before
text; old immediate Trans order/text-before-window was not final composition.
Previous MC hash only proved a stable submission-order reference. Actual outer
loop submits previous parity before increment/reset; full presentation and
clear policy need integration. Do not claim old MC composition as original OT.

Canonical camera gate139C88 reads D747A78=Work+5D8, not +584. ActualR020 Init
sets F00C6000 there, so follow-player logic really skips. The unfrozen draft
prefix used the wrong flags word; worker corrected source/tests/evidence before
integration and now restores projection138740 with borrowed Root display state.
Never manufacture tracked pose=Work.player to bypass this issue.
