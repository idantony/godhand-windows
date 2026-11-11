# Verified progress

This is an early source port, **not a playable Windows game**. The chosen route
is readable recovery of original game code with native platform adaptation.
See [progress.json](progress.json) for the latest verified, machine-readable state.

## First presented player hit in R100

A hidden, muted Windows run uses synthetic gamepad input to approach an
original enemy and start Gene's Square attack. The original hit applies 3
damage to the selected enemy. Frame 1,637 records HP 74 and frame 1,638
records HP 71 and the matching Take event. Both frames reach the window
after the room fade. The run continues to tick 1,784 and scene frame 1,643,
with 1,320,495 world packets and zero renderer, GPU, postprocess or window errors.

The next original call is the OM324 actor destructor at `0x00198CE0`.
Continued combat, room transitions and full save/load remain under development.

隐藏、静音的 Windows 实跑通过合成手柄输入接近原版敌人，并让 Gene 按方块出拳。
第 1637 帧记录敌人 HP 为 74，第 1638 帧记录原版受击事件、3 点伤害和 HP 降到 71。
两帧均在房间淡入完成后呈现。运行继续到 1784 ticks 和第 1643 帧，共提交 1320495 个
世界绘制包，渲染器、GPU、后处理和窗口错误均为 0。

下一处原版调用是 OM324 物件析构 `0x00198CE0`。后续战斗、房间切换和完整存读档仍在开发。

## First visible R100 room frame and native player movement

A hidden, muted Windows run reaches tick 1,744 and presents scene frame
1,603. Synthetic gamepad input follows the original mapping into Gene's
run state. The first movement reaches the window submission at frame 1,599,
under the fade. At frame 1,603 the final displayed composite has visible
room geometry, Gene, nearby enemies and HUD. The original fade is still active.
The run submits 1,243,415 world packets with zero renderer, GPU, postprocess
and window errors.

Two enemy actors execute the original scenery-contact reaction. The next
unfinished call is attached enemy footstep playback at `0x002CBF78`.
Continuous gameplay, player combat, room transitions and save/load remain
under development. The screenshots and original assets remain local.

隐藏、静音的 Windows 运行到 tick 1744，并呈现第 1603 个场景帧。合成摇杆输入经原版
映射进入 Gene 的跑步状态。位移从第 1599 帧提交到窗口，第 1603 帧的最终画面开始显示
房间、Gene、附近敌人和 HUD，淡出仍在进行。共提交 1243415 个世界绘制包，渲染器、
GPU、后处理和窗口错误均为 0。

两名敌人执行了原版场景物件碰撞反应。当前停在敌人的脚步声请求 `0x002CBF78`。
连续操作、玩家战斗、房间切换和存读档仍在开发。截图与原始资源留在本地。

## Complete native R100 opening and return to the room

An immutable Windows build plays all ten original EV01 camera cuts from the
patched ISO. The recovered quit path retires the opening camera and event
actors, then returns to the R100 scene loop. The hidden, muted New Game run
reaches 1,735 ticks, presents 1,594 scene frames and submits 1,224,912 world
packets. Renderer, GPU, postprocess and window error counts are zero.
The matching native suite passes 355 C modules and 349 checks, with muted
audio devices, movie decoding, two ADX sample comparisons and both RNG suites.

The late opening includes the original costume meshes, animation, scene
lighting and attached effects. One EL31 stream keeps the original zero TEX0
write and samples the previous complete frame's GS display-copy texel. Its
texture color is modulated by the vertex color while alpha comes from the vertex.
Final composites at scene frames 1232 and 1238 show the original characters
and camera change. The native model test also checks this sampling path
against independent RGBA and depth calculations.

The run reaches R100 Scenario task registration `0x002C3890` after EV01 cleanup.
It used synthetic menu input. Continuous player control, combat, room
transitions and save/load still need verification.

固定 Windows 构建从补丁 ISO 播完 EV01 的十个原版镜头，清理开场相机与角色后回到
R100 场景循环。隐藏、静音的“新游戏”运行达到 1735 ticks，呈现 1594 个场景帧，
累计提交 1224912 个世界绘制包，渲染器、GPU、后处理和窗口错误均为 0。
对应原生回归通过 355 个 C 模块和 349 项检查，包含真实静音音频设备、片头解码、两组
ADX 逐样本对照和两组随机数验证。

开场后段包含原服装网格、动画、场景灯光及附属特效。EL31 的一条网格保留原始零值
TEX0 写入，从上一完整帧的 GS 显示拷贝取样。纹理 RGB 受顶点颜色调制，透明度
取自顶点。已检查场景帧 1232、1238 的最终画面，可见原角色和镜头变化。原生模型测试
用独立的 RGBA 和深度计算核对了该采样路径。

运行完成 EV01 清理后到达 R100 的 Scenario 任务注册 `0x002C3890`。菜单使用合成
输入，连续操作、战斗、房间切换和存读档仍待验证。

## Native opening through the first camera cut

The immutable source snapshot passes **316 C modules and 340 checks**, including
muted real audio devices, movie decoding and both independent ADX sample
comparisons. Both RNG suites pass against the same native DLL with 20,544 and
20,534 exact comparisons.

The matching native Windows New Game run reaches 632 ticks, completes the
first EV01 camera cut and enters cut1/frame0. It presents 512 scene frames,
including earlier title and room startup frames, and submits 134,036 world
packets. Renderer, GPU, postprocess and window errors are zero. Final composites
at scene408 and scene457 were visually inspected. The original fade reveals the
street and sky, and the effect moves between the captured frames.

Original model-backed effects use the room's existing model/registry owners,
nested emitters and ordered sound/model cleanup. Framebuffer warp validates
the texels consumed by covered, source-dependent fragments. The current camera
reaches actor drawing and attached lighting. Shadow alignment uses its
independent original Up vector.

The next original frontier is Event actor Move `0x002B6930`, state4 in the
second camera cut. Counters through that stop are player1, child141 and
object9654, with 14 active emitters and 102 particles. Physical gameplay input,
later Event cuts, combat, progression, transitions and save/load remain active
work. Captures and resource buffers stay in private local evidence.

固定源码副本通过 **316 个 C 模块和 340 项检查**，包含真实静音音频设备、片头解码及两组
独立 ADX 逐样本对照。同一原生 DLL 的两组随机数验证分别完成 20544 和 20534 次精确比较。

匹配的 Windows 原生“新游戏”运行达到 632 ticks，完成 EV01 第一个镜头，进入第二个
镜头第 0 帧。共提交 512 个场景帧，包含之前的标题和房间启动帧，累计 134036 个世界包。
渲染器、GPU、后处理及窗口错误均为 0。已检查场景帧 408 和 457 的最终合成画面，原版
淡入揭示街道与天空，两次捕获之间可见特效位移。

模型特效共用原房间的模型和注册表，保留嵌套发射器及声音、模型的有序销毁。
画面扭曲按实际覆盖且依赖纹理的像素验证采样，当前活动相机接入绘制和光照，
阴影仍读取其独立 Up 向量。

下一原调用为第二个镜头中的剧情角色更新 `0x002B6930`，状态 4。截至该处累计完成
玩家更新 1 次、附属模型更新 141 次和物件更新 9654 次，活动发射器 14 个、粒子 102 个。
后续镜头、物理输入、战斗、完整流程、场景切换和存读档仍待完成，截图与游戏资源保持本地私有。

## Previous native R100 world layer beneath the opening fade

An immutable source copy passes **288 C modules / 318 checks**, with muted real
audio, movie decoding, full independent ADX comparison and both same-DLL RNG
checks enabled. A matching actual New Game run reaches 490 ticks / R100 Main4.
The first original scene update/render cycle completes and submits 1,858 native
world geometry packets, with renderer/GPU/window errors all zero.

The world surface before UI was read back and visually inspected: Gene, R100's
street/buildings, sky, textures and shadows are present (228,967 nonblack pixels,
31,463 RGB colors at 512x448). The final composed frame is opaque black because
the original opening Fade is visible with RGBA `FF000000`; no visible interactive
window or continuous gameplay is claimed. The prior 369 title UI frames plus
this one R100 cycle give 370 presented scene frames. Image/resource buffers stay
private; public source contains no original game assets.

The next tick stops at original OM304 state1 `0x00188190`, SceneStage13. Counters
include that partial tick: player1 / child2 / OM81, four ambient emitters and
zero ESP slots. The first cycle included all69 cold objects and four runtime
3EB tutorial objects. Native walk/run animation and root displacement, original
ESP update/dispatch/lifetime, unlit material constants, signed-W clipping and
full packed16 XY raster coverage now have focused tests. Physical analog input,
positive effect postpasses, continuing actor/combat states and progression remain
active work; independent numerical profiles are not PS2 hardware traces.

固定源码副本通过 **288 个 C 模块／318 项检查**，含真实静音设备、片头解码、完整 ADX
独立对照和同 DLL 两组随机数验证。匹配运行进入 R100，完成一次原场景更新／绘制，
提交 1858 个世界包；渲染、GPU、窗口错误均为 0。合成前世界层已实际读回并检查，角色、
首关街道建筑、天空、纹理和阴影均已出现。最终帧仍被原开场 `FF000000` 淡入遮罩覆盖。

运行共 490 ticks，370 场景帧包括 369 个标题帧和一个 R100 周期；下一 tick 停在原物件
304 状态1 `0x00188190`。累计玩家／子模型／物件更新为 1／2／81，包含第二 tick 的部分
进度。尚不可玩，未验收连续动画、物理输入、战斗、切换和存读档；图像和游戏数据仍保留
本地私有，不进入公共 Git。原行走／跑步根位移、粒子生命周期、共享材质参数、裁剪和
完整 16 位坐标覆盖已有分项验证，不将这些数值参考称为 PS2 实机逐位证明。

## Previous milestone: SceneStage22 model-hide/reset frontier

An immutable source snapshot passes 270 C modules / 295 checks with hardware-muted
audio, movie decoding and the full independent ADX comparison enabled. The hidden,
muted synthetic New Game run completes 489 ticks and reaches R100/Main state4.
It completes one original player update, one child update and 73 original object
updates: all 69 cold OMS objects and four runtime tutorial objects from 3EB.
The first original FX call 0x30F228 completes and creates four ambient emitters
(bank3; no active ESP slots). Two RNG routines in the same DLL pass 20544 exact
state/float-bit comparisons across 10272 seeds against a restricted model of
the actual instructions.

The next actual frontier is 0x00308BC0 inside model-hide/reset call 0x00307B28,
SceneStage22. The run submits zero world packets. No world scene frame, playable
scene, continuous animation or combat has been verified. Hardware/device and
decoder checks are enabled; exit code 4 remains a normal unfinished frontier.

固定源码快照通过270个C模块／295项检查，启用硬件静音音频、片头解码和完整独立ADX对照。
隐藏、静音的合成“新游戏”运行完成489个tick，进入R100／Main state4。实际完成玩家更新1次、
子模型更新1次和原物件更新73次：包括全部69个冷启动OMS物件及来自3EB的4个运行时教程物件。
首个原始FX调用0x30F228完成，并创建4个环境发射器（bank3；ESP 活动槽为 0）。同一DLL中的两个
RNG函数对10272个种子完成20544次状态／浮点位精确比较，依据实际指令的受限模型验证。

下一实际边界为SceneStage22中模型隐藏／重置调用0x00307B28内的0x00308BC0。运行提交的世界
绘制包为0；尚未验证世界场景首帧、可玩场景、连续动画或战斗。已启用硬件／设备和解码器检查；
退出码4仍表示正常停在未完成边界。

## Previous milestone 67: complete first player and child updates

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
