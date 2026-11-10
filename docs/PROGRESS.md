# Verified progress

This is an early source port, **not a playable Windows game**. The chosen route
is readable recovery of original game code with native platform adaptation.
See [progress.json](progress.json) for the latest verified, machine-readable state.

## Current milestone: complete first player and child updates

The immutable source copy passes 254 C modules / 273 checks, including real
muted audio, movie decoding and the full independent ADX comparison. Its
matching Windows run completes one original player108AB0 update and one
child100530 update. The actual counters show all child force, angle, distance
and finish stages completed. The next unresolved descriptor is the first
door's Move method `0x00186928` (handle159).

This connects the original motion/blend, interaction, solid/wall/floor collision,
ground pose, hierarchy/IK, opacity, position history and Save/Heat updates.
The child uses the same parent parts and persistent spring/template owners.
A real one-ULP acos input exposed an EE COP1 SQRT difference; its explicit
numeric reference follows the original operation, with no input clamp or
PS2 hardware-equivalence claim.

The run still submits zero world packets and stops during MoveObj. It is not
a complete scene frame, continuous animation, combat or a human playtest.
Focused attached-sound tests additionally verify real muted random-pitch ADPCM
voices; complete in-game audio remains unfinished.

固定源码副本通过 254 个 C 模块／273 项检查，含真实静音音频、片头解码和完整 ADX
独立对照。匹配的 Windows 程序实际完成玩家与附属子模型各一次完整更新，弹性骨骼
各阶段均完成；下一处为首扇门 Move `0x00186928`。世界绘制包仍为 0，尚无完整场景
首帧、连续动画或玩法验收。数值参考遵循原 CPU 运算语义，不以夹住输入掩盖差异，
也不声称 PS2 实机位精确。原声音、战斗、存读档和完整流程仍需继续恢复。

## Previous milestone: player first-update motion

An immutable source copy passes 221 C modules / 237 checks, including muted
hardware audio, the movie codec and the full independent ADX comparison. Its
matching Windows QA executes 489 updates: 6 MC, 90 movie and 369 title/menu
frames. Main state4 now completes original NearCheck, world lighting and
positive HitArea sorting, actor filtering, line-of-sight and hit publication.
It then enters the actual player Move method, selects idle motion, binds the
original 10-frame blend and samples its first pose. The next unresolved call
is idle input/interaction `0x00123938`.

Independent motion tests exercise 12 ticks and 32 original PL00 parts, actual
alternate-idle SEQ timing, old-sequence release and late-error resumption. This
is not continuous in-game animation or a human playtest. World packets remain
zero, so the original 3D scene, complete player frame and gameplay are pending.
Enemy initialization and effects work are being verified separately and are
not included in this integrated execution claim.

Self-built evidence tools check instruction bytes against the user's actual
ELF, classify direct calls/imports and summarize static memory operands. The
Ghidra exporter now records instruction limits and truncation explicitly.
See [tool documentation](EVIDENCE_TOOLS.md); raw reports stay private.

固定源码副本通过 221 个 C 模块／237 项检查，含真实静音音频设备、片头解码和完整
独立 ADX 对照。匹配的 Windows 验收完成 489 次更新：6 帧存储卡、90 帧片头和
369 帧标题／菜单。Main state4 已完成 NearCheck、光照和正碰撞排序／对象筛选／
视线／命中发布，真正进入玩家 Move，执行原待机选择、10 帧过渡绑定及首次姿态采样。
下一调用为输入／交互 `0x00123938`。世界绘制包仍为 0，没有完整首帧或可玩场景。
独立动作测试覆盖 32 个部件、12 次更新和失败续接，不能替代整合动画验收。自制工具
核对实际指令、调用／导入及访存操作；大函数截断会明确标出，原始报告不公开。

## Previous milestone: R100 Init and first update entry

An immutable source copy passes 212 C modules / 228 checks with muted hardware,
codec and the full independent ADX comparison. Its matching Windows QA completes
489 updates: 6 MC, 90 movie and 369 title/menu frames. Original R100 Init now
finishes, including all69 OMS objects and four real runtime tutorial objects;
Main state4 enters its first update and stops at NearCheck ray `0x00131E10`.
Roof/intro Scenario tasks are registered but have not been visited yet.

The native input sample now supplies original port0 readiness from the keyboard
adapter; losing focus releases keys without unplugging it. Collision dispatch,
dynamic boxes and shared static/dynamic ray queries pass real-resource tests.
Original room music selection independently reaches real muted PCM consumption;
it has not yet started in this bounded Windows run. No world packet is submitted,
so no original 3D frame or gameplay is claimed. Later updates, Event/actor logic,
continuous animation, combat and save/load remain unfinished.

固定源码副本通过212个C模块／228项检查；匹配的Windows程序完成489次更新，显示
6帧存储卡、90帧片头和369帧标题／菜单。R100初始化及69个OMS／4个教程物件完成，
Main state4首次进入更新，停在NearCheck射线入口0x00131E10；Scenario任务已注册，
尚未遍历。键盘适配器写入原输入连接状态；失焦与断开分开处理。碰撞／混合射线和
首关BGM正分支已分别测试，不能当成整合画面或玩法验收。世界绘制包仍为0。

## Previous milestone: original player and first-room objects

An immutable source copy passes 196 C modules / 207 checks with muted hardware,
codec and full independent ADX verification. Matching hidden Windows QA executes
the original Title/New Game transition into R100/Main3: 487 updates, 6 MC,
90 movie and 368 title/menu frames. All 69 original OMS object initializers
complete after 156 static constructors, full player/child construction, initial
motion sampling, terrain IK, matrix recovery, spawn and camera binding.

The next actual call is the R100 roof task at `r100.rel+0x2F30`. No world packet
is submitted, so there is still no original 3D game frame or playable game.
Separate palette/geometry tests transform 9573 original vertices with canonical
part matrices; this does not establish lighting, clipping or a rendered frame.
Collision dispatch and unequal cylinder argument regressions now inspect the
actual ELF call table. Positive queries outside the recovered domain stop.
Numerical reference profiles and modeled checks are not PS2 hardware captures.

固定源码副本通过 196 个 C 模块／207 项检查；匹配的隐藏静音 Windows 程序完成
487 次更新，显示 6 帧存储卡、90 帧片头和 368 帧标题／菜单。实际运行完成玩家及
子模型、初始动作采样、地面 IK、矩阵回写、出生／摄像机绑定，以及 156 个静态对象
和全部 69 个 OMS 物件初始化。下一边界为屋顶任务 `r100.rel+0x2F30`。
世界绘制包仍为 0，尚无首关 3D 画面；独立骨骼矩阵／几何检查不能代替实际绘制。

## Previous milestone: integrated original title scene

An immutable source copy passes 105 C modules and 111 checks, including muted
device/codec execution and the full independent ADX comparison. Its matching
Windows QA binary completes 600 updates: 6 MC frames, all90 Clover frames,
then481 original title scene frames with no unresolved consumer. A captured
frame shows GOD HAND / PRESS START. Original timers advance the logos; title
input is not injected in this bounded acceptance run.

The actual OT traversal now orders logical banks, depth buckets and LIFO packet
groups. Text stays one packet group; each numeric digit is separate. Completed
pixels are presented on the following update, before borrowed resources expire.
Canonical cameras, display state, object registries, scene updates and HUD
consumers share their owners. Explicit zero-memory and binary32 reference
policies remain separate from PS2 hardware validation.

A separate New Game QA reaches the difficulty menu and then the actual bank2
sound boundary2CB8A0. State5/SystemMenu/TitleEnd and sound release are covered by
bounded original-owner tests; the integrated first world scene is still pending.
No original combat, player animation, full progression or save/load is claimed.

当前固定源码副本通过105个C模块/111项检查；匹配的Windows程序完成600次更新，
显示6帧存储卡界面、90帧片头和481帧原标题场景。已捕获PRESS START画面。
另一“新游戏”验证到达难度菜单后停在真实bank2音效入口；第一关和可玩玩法仍未完成。
以下条目保留历史节点，最新范围以本节及progress.json为准。

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


## Local continuation: scene setup, native ADX and direct title UI

84 C modules pass85 checks with device/codec/reference opt-ins. Hidden Windows
startup reaches002BF8F8 after native world-light defaults, model-packet reset,
Cockpit room reset, collision/table setup and the original title-player
constructor. Earlier boot, the rest of case3 and complete state4 remain pending.

Native encrypted ADX decoding matches all6369200 stereo sampleframes against an
independent vgmstream output. Bounded native streaming passes real muted device
consumption, pause, loop, seek, stop and EOF checks. Original BGM request Set,
Start and Stop execute supported actual paths; unsupported paths remain explicit.
The original logo SFX uses its real bank/program/sample and device lifetime.

A separate direct R020 Init/Move GPU fixture renders the original warning
screen using its actual IDD. It has an explicit512x448 black reference clear;
it does not bypass missing scene operations in production, establish final OT
order, or demonstrate a playable game. The earlier MC first-frame hash remains
unchanged after reusing the renderer and adding the rotated polygon backend.

本地检查点：84个C模块通过85项检查，隐藏Windows主流程到达002BF8F8。原加密ADX
逐样本验证和真实静音设备队列通过；原标题Init/Move的独立GPU验证已画出警告首屏。
完整case3/state4、最终绘制顺序、摄像机、模型/动画、玩法和存档仍需继续，不宣称可玩。
