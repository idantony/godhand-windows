# Verified progress

This is an early source port, **not a playable Windows game**. The chosen route
is readable recovery of original game code with native platform adaptation.
See [progress.json](progress.json) for the latest verified, machine-readable state.

## Milestone 1: original input and native foundation

- Indexed the user's patched NTSC-U disc: 1445 resources and 81 dynamic modules.
- Implemented native C read-only original-disc access and verified an actual
  room resource byte-for-byte without bulk extraction.
- Recovered two RNG functions, one boot dependency slice and six small room
  functions into native C, with explicit original addresses and provenance.
- Built native x86-64 code and passed RNG, boot, resource, room and C++ linkage
  checks. No missing platform operation silently returns success.
- Compared two RNG routines against a restricted model executing instructions
  read from the actual ELF: 10272 seeds, 20544 exact state/float-bit comparisons.
  This is modeled evidence, not PS2 hardware validation.

## Work still required

Complete the original frame loop, title/scene and model/animation consumers,
remaining audio/input/save services, then verify original combat, transitions
and progression. Native startup, MC and Movie0 integration are described below.
The current room slice covers its small allocation/init/move behavior only; it
does not render or enter a playable room.

The upstream project's PS2 matching percentage is not our Windows-port
completion percentage. The patched boot ELF differs from its retail target;
all claims must remain tied to the actual input and verification scope.

## Milestone 2: lifecycle and real host memory

- Recovered r207's three remaining lifecycle routines from its actual zero-count
  constructor table and empty destructor list. This exact module has no observed
  nonempty constructor/destructor sequence; no general CRT was invented.
- Added real native aligned allocation, bounded payload capacity, ownership and
  explicit error handling. The room initialization test allocates/writes/releases
  actual host memory. This does not claim complete original cHeap behavior.
- Recovered three task-manager functions: slot selection, attribute inheritance
  and filtered visitation, including changes to flags/count during traversal.
  This milestone did not yet connect thread step/start/wait/kill.
- Build evidence fingerprints the actual inputs; progress export rejects changed
  sources or a stale DLL differential result instead of reusing old pass labels.

## Milestone 3: native startup-task integration

- Real Windows fibers implement the observed serial task start/sleep/exit
  handshake. Unsupported EE kernel behavior remains explicit.
- DVD jobs read actual sectors into owned aligned allocations, including real
  sector padding. Priority selection, queue-pressure drain and the unconditional
  extra WaitFrame sleep are recovered from the actual executable.
- Recovered task `0x0030D358` runs with real native sound-bank, DVD, font, menu
  and ID adapters. An integration run reaches the next task request after five
  scheduler ticks; these are not timed video frames.
- Native sound startup owns program/sample tables and indexes actual external
  AFS directories, including BGM0. It does not synthesize RPC acknowledgments,
  execute voices or produce audio output.
- Default next entry `0x002AE8A8` is the memory-card boot task. A literal `jal 0`
  inside it is resolved by SNR2 relocation to `mcport.rel`'s `Main__7cMcBoot`;
  treating it as a no-op would skip original behavior. The host reports an
  explicit frontier instead of installing an empty task.
- Original texture RGBA passes native decoding and RTX offscreen upload/readback.
  GPU asset QA does not establish game rendering or gameplay.
- Restricted original font packets are decoded through CT32 upload storage to
  PSMT4 sampling. The original code's odd/even glyph CLUT selection is preserved;
  native glyph RGBA is distinct from final screen-space drawing and blending.
- The later `0x0030D5C8` loader separately passes real core, room-jump,41-object
  table and all eight player-costume read checks. The table is independently
  checked against the actual ELF; this is not full GameMain execution.
- Native memory-card queries and recovered slot/file checks run against an
  actual host profile directory. Localized/fallback idsavecore loads bind slot11
  without replacing slot1. Full MC Main still needs IDBase/UI and save semantics.

## Earlier local continuation: original GameMain case0

- The alternate original flags branch now registers the recovered GameMain and
  asset-loader tasks. Real no-draw gates and all eight case0 subsystem
  initializers run; the loader finishes concurrently before the host reports
  the next unavailable state1 handler (`0x002A5BB8`). This is a developer branch
  execution slice, not the default complete game startup.
- Original CoreSave system/game initialization preserves fields according to
  the actual writes. Enemy/object pool bounds, linked-head reset and the
  separate624-word random generator are explicitly recovered and tested.
- Actual67-entry MC IDBase initialization, typed pools, parent linking/reset
  and the packed-message consumer pass original-resource tests separately.

原 GameMain 的case0与并行资源任务已能在开发宿主中真实执行，接下来停在case1。
默认存档启动路径已连接真实模块核验、图标分配、67个ID初始化、packed环境和
初始DispInit，停在模块Control循环；仍未到达可玩场景。原T32存档素材已解码，
ID的实际移动、颜色动画、绘制顺序和受限纹理quad也已分别验证；完整窗口绘制待接。

The development host begins at the recovered startup task. Earlier boot
initialization, the complete main loop and the actual next tasks are still
required. No playable scene or controls have been demonstrated.

## Milestone 4: native Windows MC, movie and Main continuation

- A single hidden native HWND/D3D11 run presents six actual MC interface frames,
  executes normal MC cleanup and same-slot tail restart, then presents all90
  Clover movie frames. Real videoEOF and audioDONE complete before continuation.
- The run uses an explicit synthetic digital input fixture, through the original
  ELF table. It is not a human input/playability claim. QA output is muted while
  the decoder/device queues and completion tracking actually run.
- Original GameMain setup advances through states0/1/2 into3, using canonical
  save, camera, HUD, room and task owners. Actual r020 title-room resources and
  RHT-based pools load. The precise next frontier is main ELF002BE708, native
  room overlay binding; no title scene or combat is claimed.
- The MC GPU consumers preserve actual draw order, original palette, font
  nearest sampling and separate T32/TIM2 four-bit bilinear sprite math. Real
  cursor and window pixels match independent CPU equations. Reusing the common
  renderer preserves the initial MC frame's byte hash.
- All55 C modules and56 checks pass with hardware/codec opt-ins enabled. An
  independent clean staged-source foundation snapshot also passes55/56. Separate
  Windows targets rebuild every C module and fingerprint all consumed sources.
- The normal native target accepts actual window/device input. Synthetic keys
  exist only in the separately compiled startup-QA target. Unknown consumers
  remain explicit frontiers. Earlier boot and complete game frame timing remain
  unverified; the executable is not yet a playable game.

## 中文进展摘要

同一个 Windows 原生程序已串联原存储卡可见界面、正常退出、Clover 片头和主游戏初始化，
真实加载标题房间资源后停在002BE708模块绑定。隐藏静音验收显示6帧存档界面和90帧片头，
使用明确标注的合成按键；视频EOF及音频设备DONE均实际完成。55个C模块通过56项检查。
完整主循环、可玩场景、模型/动画、战斗和存档读写仍未完成，不以此宣称玩家实测或移植完成。
当前测试和限制以 [progress.json](progress.json) 为准。

## Local continuation: original scene setup and outer-frame leaves

The same hidden native startup now reaches002D6F40. It actually loads and
version-binds R020's module identity, performs the preceding Scenario leaf,
commits the real room SE bank, initializes collision records, effect resources
and room filter presets. R020's exported Init/Move have not been claimed as
executed.64 C modules/66 checks pass; Windows QA still presents6 MC/90 movie
frames before the precise new frontier.

Known outer-loop order now places Movie update before tasks and Fade after
tasks, ahead of DVD/audio. Recovered frame policy preserves the separate FPS
and mode bytes. The observed boot seed and two-word MT advance share the actual
GameInit MT owner. The finite conversions pass20534 comparisons across10267
supplied MT words against their actual restricted instructions. This reference
is independent of host rounding and remains distinct from hardware/full-boot
validation. New Fade GPU primitives are available; unknown frame surfaces remain
explicitly unsupported.
