# Active project resume: God Hand

Updated 2026-11-10. Read Git and live worker ownership before editing. This is
an ongoing authorized source port, not a playable Windows game yet.

## Scope and authority

- Recover readable original behavior and native Windows platform support, using
  OpenGOAL's method. No remake, emulator packaging or generic translated runtime.
- E:/godhand_windows holds useful source, tools, references and evidence.
  G:/middle_thing/godhand_windows holds builds/cache/headless projects/large logs.
- User authorizes needed tools, GPU and subagents. Developer agents at most
  gpt-6.1-sol; bounded mechanical work prefers gpt-6-luna. No Codex Fast.
- Public repo https://github.com/idantony/godhand-windows. Local commits provide
  rollback; push meaningful integration milestones. Formal overall review and
  private workload estimate follow GitHub pushes, per latest clarification.
  Never publish the subjective percentage or equate upstream matching with it.
- README is bilingual. Exclude game data, raw disassembly/decompiler dumps,
  binaries/dependencies/credentials. Keep reusable skill in its own folder.
- User says keep working until fully complete while away. Goal is active with
  no requested token budget. Status questions do not cancel it. Do not mark
  complete at a build, module or push; do not pause without instruction.
- Historical authority applies only to this game/task, not another game or a
  new read-only request. Hidden/muted QA must not steal focus/change volume.

## Baseline and tools

ISO: E:/godhand_windows/God Hand (USA)(Uncensor Patch v1.0).iso
SHA256 215a145a8575f9708b8344e924bd68d20980f9c1afa334ed6f3e5148df66b1dd
ELF: reference/SLUS_215.03
SHA256 3192ce89412d028a428970d3fcf58a56514878810e7e0608f2684966b10a3b95
Upstream MIT pin: eaae18d0dfab1f9a0df276ae5ca1581ad326faf3.
The patched loaded ELF differs retail; all81 overlays match pinned manifests.

Zig .tools/zig-x86_64-windows-0.17.1/zig.exe; Ghidra12.1.4/EE Reloaded2.1.38/
JDK21; REA6.8 Windows backend rejects PS2 ELF, so use standalone Ghidra.
Windows SDK x64 um libs: C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64.
FFmpeg .tools/ffmpeg-9.0.2-essentials_build/bin/ffmpeg.exe; vgmstream-r2168
independent oracle. G:/middle_thing/godhand_windows/adx-research/reference.wav
matches all6369200 stereo48k title samples. RTX4080SUPER hardware D3D11 tested.

## Latest whole verification

Immutable snapshot G:/middle_thing/godhand_windows/snapshots/room-transition-22
passes151C/159checks, hardware/codec/fullADX enabled. RNG20544/20534 modeled
comparisons pass. Matching hidden/muted startup-QA NewGame runs470updates:
6MC/90movie/351title-menu frames; Title -> actualMain5 cleanup -> R100/Main3.
It constructs156static objects,193actor-scrolllinks (147canonical/46original
misses), actualplayer100 factory, full parentPL00(32parts/26nodes) and childPL01
(41parts/2nodes)149738; child100420 hierarchy/40springs, attach17, shadow,
solidunit/sphere and HP execute. Next strict originalcall001FE218. Worldpackets0.
No renderer/window/cleanup errors; process exit4 means unfinished frontier.

Private analysis/room-transition-22-verification.json records exact scope.
Standard native-verification.json, windows-startup-qa-build.json and
native-startup-window-verification.json now identify22, not oldTitle04.
Separate world-qa/model-qa in15 pass GPU pixel equations; these numeric fixtures
are not a runtime world frame or EE/VU/GS hardware proof. Title04 previously
completed600updates/481titleframes; screenshot under reference/render/.

Earlier17 is NOT whole-pass (stale ScenePost assertion);20 is NOT whole-pass
(dedicated GameInit test missed new link dependency). Preserve failed receipts.
21 passes149C/157checks, strict100420. Do not modify any immutable source copy.

Git history before the next push: publicd4fad08 title, local372e13d NewGame,
7e2568a first-room constructors/native static renderer. Reconcile current Git;
new exact22 checkpoint/docs may already have been committed/pushed.

## Ownership and immediate next work

- Root owns build manifests, platform/game_services/startup_host/main and native
  GPU integration, exact-source verification, curated docs/checkpoints/push.
- godhand_upstream owns actor models/packets/hierarchy/matrix and child-tail;
  next firstdoor300 virtual1B7118. Matrix batch after22 is separately frozen,
  not yet whole verified: add game_actor_matrix.c; coordinate shared headers.
- opengoal_method owns SceneAt geometry/step, follow/envelope/shake; new original
  player spawn31C5C0 in progress. It must follow real107F58 completion.
- skill_forward_test owns HitArea/shape constructors and player-init tail. New
  collision_pair_shapes/hitarea_shapes need SAMEinit/pairs and Rootmath owner;
  existing shared-file edits are coordinated/frozen per batch.

Use collaboration live status: a send_message to a completed worker does not
wake it; use followup_task. Never stage all newer working changes. Workers may
have added files/ABI changes that are not in the last passing source copy.

Production ONEowners: Root camera follow/envelope, math_globals (3079C8 static
init once), mutable3BD880 player_shared_vector and event/display; services borrow.
services.world_light.scalar+4 is actual604704 fog, models borrow sameheap/pools.
Child tail publishes typed Work5F8 only after real hierarchy/springs; parent
Work5F4 is canonical factory player. Never truncate pointers into PS2 words.
Shutdown clears attachment/publication before child/model/arena/resource release.
Native incomplete-construction abort is not original normal actor destruction.

Keep actual SceneAt Work/two-pass unit state, real Task/Door/RoomSave callbacks;
missing consumers remain strict, no successful placeholders. Current player
initialization blocks before later31C5C0/R100Init/OM methods/Event scripts.
R100 script9B0 requires real Event1, not FMV: ev01.dat custom010A0900 3D data;
Event.rel Release aliases debugoptimize. Resolve module imports before naming
JAL0 empty. Private r100-event-*-ghidra reports retain investigation; config
2975F8/297660/299320 has latest event-config-02 evidence, no new runtime yet.

## Non-obvious corrections

-131E70 stack20==0 -> straight132140; nonzero -> segmented8units/1%overlap.
 Camera and SceneAt use segmented1/T3=0; ScenePost uses straight0.
- Player2B4 is partcount32, not carried flag. Follow envelope borrows modelparts.
- Actor190/194 is LIVE dynamicshape SOURCE despite legacy pending_actor name.
 130700 extra callerargs are OUTPUT identities/capacity/count. Positive live
 source remains strict130700; never manufacture a miss.
- Triangle visitedbit4 companion merges mutable canonical flags (roofbit2).
 Actual triangle unusedfourthindexFFFF is not a fourth vertex. Typed reports
 replace guest-pointer outputs; native failure defers caller output commits.
- Fixed-field pointer companions overwrite byoffset. HUD appended assignment
 history until capacity160; fixed code passes8reset/quit cycles405entries39runs.
- OT banks/depth/LIFO plus previous-parity presentation determine actual order.
 Static world geometry+setup are TWO groups in ONEOT, setup captured fog byvalue.
 World/UI currently admits world-before-UI, UI ZTSTalways; reject interleaving.
- Native binary32/FEnv and uninitialized-memory profiles are explicit scoped
 adaptations. Read godhand-case.md; no console-equivalence or fake HP100 claims.

## Repeatable verification

python -B tools/freeze-source.py --destination G:/middle_thing/godhand_windows/snapshots/UNIQUE
Optional --base-snapshot PRIOR --overlay relative/path (repeatable) selects a
coherent frozen batch while other workers continue. Never edit snapshot sources.
Build tools accept --scratch G:/.../verification/UNIQUE and --jobs4; objects are
fresh, Windows toolchain cache is content-addressed/shared. Device tests serial.
Use build-native.py --hardware-audio --movie-codec FFMPEG --adx-reference-wave WAV;
build-windows.py --target startup-qa --sdk-lib SDK; then hidden/muted --qa-new-game
--max-ticks1100. Record raw stdout/stderr/exit and exact executable/build hashes.
Also verify_rng.py and verify-frame-rng.py. All intermediates stay onG.

stage-verified-snapshot.py --snapshot PATH --windows-target startup-qa stages
exact passing input blobs with Git clean filters, without overwriting newerE
edits. Extra authored docs use explicit staging. export-progress.py
--verified-source-root PATH validates frozen inputs while workers continue.
check-public-tree.py scans INDEX; no git add . or originaldata. Inspect staged
diff before commit. Formal global review/private estimate follows milestone push.

Skill validation: python -B C:/Users/sutong/.codex/skills/.system/skill-creator/scripts/quick_validate.py E:/godhand_windows/skill/ps2-source-port
Acceptance remains full native scene/render/input/originalplay/combat/audio,
transitions/save-load and progression. Compile/constructor proofs are partial.
