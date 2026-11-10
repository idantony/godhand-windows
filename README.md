# God Hand — Windows Source Port / Windows 源码移植

[English](#english) · [简体中文](#简体中文)

## English

Recover readable source for the original PS2 **God Hand**, preserve its gameplay
and assets, and replace platform dependencies with native Windows implementations.
This follows OpenGOAL's source recovery/platform adaptation method. God Hand was
not written in GOAL, so Jak's specialized decompiler is not directly applicable.

**Early development; not playable yet.** The original R100 world layer now
renders natively from the user's ISO: Gene, the street/buildings, textures and
shadows have been visually inspected before UI composition. One complete
original scene update/render cycle submits 1,858 world packets without renderer,
GPU or window errors. The final displayed frame is still covered by the
original opaque opening fade.

The hidden, muted synthetic New Game run reaches R100 / Main state4 and stops
at `0x00188190` in an original `304` object state on the next tick (490 ticks).
It presents 370 scene frames: the earlier 369 title UI frames and one R100 cycle.
Counters include the partial second tick: one player update, two child updates
and 81 object updates. Continuous animation, physical gameplay controls, combat,
transitions and save/load remain unfinished. The run starts at task `0x0030D358`;
earlier boot and complete platform behavior are still outside this slice.

### Verified progress

- Read-only access to 1445 resources and 81 overlays from the user's patched
  NTSC-U `SLUS-21503` disc. All 81 overlay hashes match the pinned upstream
  manifest; the boot ELF and loaded image differ from its retail baseline.
- Original serial task behavior runs on native Windows fibers, including nested
  Scenario tasks and same-slot tail restart. DVD jobs perform actual ISO reads,
  priority selection, sector padding and original task waits.
- Actual MC prompts, text, window and TIM2 cursor draw through restricted D3D11
  backends. Absent-card and empty-profile paths execute original Control,
  digital input mapping, limited real SFX and normal resource cleanup.
- One hidden, muted native Windows synthetic New Game run completes 489 ticks
  in R100/Main state4. It completes one player update, one child update and 73
  object updates: 69 cold OMS objects plus four runtime tutorial objects from
  3EB. Hardware-muted audio, movie decoding and the full independent ADX
  comparison are enabled. Synthetic keys are not a human playtest.
- Limited MC ADPCM voices use real XAudio2. Movie0 uses an explicit native
  FFmpeg codec process and Windows audio. Reference DSP/GS numerical profiles
  are documented; they are not PS2 hardware-equivalence claims or complete audio.
- Original case0 initialization, HUD setup, room state, camera reset and room
  allocation operate on shared native owners. Asset descriptors are distinct
  from completed model/animation rendering or gameplay.
- Actual first-room construction creates 156 static objects and all 69 cold
  OMS objects. Parent PL00 and child PL01 share original parts/nodes, child
  springs and attachment. Original player initialization executes initial motion
  binding, bone sampling, terrain IK and matrix recovery, followed by spawn and
  camera binding. Four runtime tutorial objects from 3EB use the original
  factory and registries. The first original FX call 0x30F228 completes and
  creates four ambient emitters (bank3; no active ESP slots). This is not continuous
  animation or gameplay acceptance.
- 288 C modules pass 318 checks with hardware-muted audio, movie decoding and
  the full independent ADX comparison enabled.
  The earlier foundation also passes in an independent clean source snapshot.
  Restricted GPU pixels match separate CPU equations. The recovered OT traversal
  preserves bank order, bucket depth, LIFO packet groups and previous-frame presentation.
- The two RNG routines in the same DLL pass 20544 exact state/float-bit
  comparisons across 10272 seeds against a restricted actual-instruction model,
  not console traces.

Native encrypted title BGM matches every sample from independent vgmstream
output. Real-resource tests cover attached ADPCM voices with original random
pitch. Native world rendering preserves the original shared OT order, material
state and packed coordinates; exact wide integer coverage is independently
checked across the full 16-bit XY range. Original walk/run states and their
PL00 animation/root displacement pass focused tests, while platform analog
input is still being connected. The first R100 world layer is verified beneath
the opening fade; the next object state, continuous frames and full game
behavior still require recovery. No human playtest or playable build is claimed.

Project evidence tools verify actual instruction bytes, calls, imports and
static memory sites. Their generated reports remain private; see
[Evidence tools / 证据工具](docs/EVIDENCE_TOOLS.md).

Current boundaries: [docs/progress.json](docs/progress.json).
Milestone history: [docs/PROGRESS.md](docs/PROGRESS.md).
Upstream PS2 matching coverage is not this native port's completion percentage.

### Prepare and build

The repository contains authored source, tests and tools. Supply your own ISO.
No game binaries/assets, BIOS, private SDK, tool binaries or credentials are
included. The patched input is version-bound; a different release needs its own
baseline and source verification.

```powershell
.\tools\bootstrap-toolchain.ps1
python -B .\tools\prepare-inputs.py --iso 'X:\your\God Hand.iso'
python -B .\tools\build-native.py
python -B .\tools\verify_rng.py
```

The optional Windows presentation target needs Windows SDK x64 import libraries
for D3D11/DXGI/D3DCompiler, a hardware D3D11 device and an audio device. Movie0
requires an explicit FFmpeg executable; the tested build is Gyan FFmpeg9.0.2
essentials, distributed separately under its own license. Pass its path below.

```powershell
python -B .\tools\build-windows.py --target native
& 'G:\middle_thing\godhand_windows\build\windows-native\godhand-windows.exe' `
  --iso 'X:\your\God Hand.iso' --profile 'X:\your\native-profile' `
  --ffmpeg 'X:\tools\ffmpeg.exe' --show
```

The native profile directory represents slot0; opening does not create it.
Missing directory genuinely takes the no-card path. Save loading/writing is
unfinished. Keyboard defaults are Z=Cross, X=Circle, arrows, Enter=Start and
Escape=Triangle; original ELF tables determine logical confirm/cancel.
The window stays hidden unless `--show` is supplied. `--muted` silences only
this program's output while retaining actual device processing/completion.

Exit code **4** reports an unfinished execution frontier. It is expected at the
current unfinished scenario boundary and never means the game is complete. `--prefix`
selects the bounded first-MC-frame developer check. Scheduler visits are not
proof of complete original frame timing.

For the explicitly synthetic, hidden startup acceptance check:

```powershell
python -B .\tools\build-windows.py --target startup-qa
& 'G:\middle_thing\godhand_windows\build\windows-startup-qa\probe-native-startup.exe' `
  --iso 'X:\your\God Hand.iso' --profile 'X:\your\native-profile' `
  --ffmpeg 'X:\tools\ffmpeg.exe' --hidden --muted --qa-new-game --max-ticks 1100
python -B .\tools\build-native.py --hardware-audio --movie-codec 'X:\tools\ffmpeg.exe'
```

`--qa-confirm` and `--qa-new-game` exist only in the separate QA target. Real rendering/audio/video
consumers are required; unknown operations stop at explicit frontiers.

| Directory | Purpose |
|---|---|
| `src/`, `tests/` | Recovered source, native backends and meaningful checks |
| `tools/` | Input preparation, repeatable builds and inspection |
| `docs/` | Curated public progress |
| `skill/ps2-source-port/` | Independent reusable skill and resumable notes |
| `reference/`, `analysis/` | Private input/evidence, excluded from Git |
| `vendor/`, `.tools/` | Pinned upstream and local tools, excluded from Git |

Useful project files stay under `E:\godhand_windows`; builds, cache and large
logs stay under `G:\middle_thing\godhand_windows`. Scripts accept external
scratch/compiler paths. Original game data remains read-only. Attribution:
[NOTICE.md](NOTICE.md).

Next: continue original object states beyond the opening fade, connect physical
input, and verify continuous animation, combat, transitions and save/load. Local commits retain rollback points; public pushes
mark substantial milestones.

## 简体中文

恢复《神之手》原游戏的可读源码，保留原玩法和资源，用原生 Windows 实现替换 PS2
平台依赖。采用 OpenGOAL 的源码恢复与平台移植方法；本作不是 GOAL 程序，不能直接
使用 Jak 专用反编译器。

**仍处于早期开发，尚不可玩。** 已从用户 ISO 原生绘制并检查 R100 世界层：
角色、街道建筑、纹理和阴影均已出现。一次完整的原场景更新／绘制提交了 1858 个世界包，
渲染器、GPU 和窗口错误均为 0；最终显示帧仍被原流程的不透明淡入遮罩覆盖。

隐藏、静音的合成“新游戏”运行进入 R100／Main state4，在下一 tick 的原 304 物件状态
`0x00188190` 停止，共 490 ticks。已提交 370 个场景帧，其中前 369 个是标题界面，
随后一个是 R100 场景周期。累计计数包含第二 tick 的部分进度：玩家更新 1 次、附属模型
更新 2 次、物件更新 81 次。连续动画、物理输入操作、战斗、切换和存读档仍待完成。
程序从原启动任务 `0x0030D358` 开始；更早的启动和完整平台行为尚未全部恢复。

### 已验证的进展

- 从用户补丁镜像只读索引 1445 项资源和 81 个动态模块。模块哈希与固定上游一致；
  补丁主程序和加载映像与上游零售版不同，不能照搬其匹配结论。
- Windows fiber 实现原串行任务、嵌套 Scenario 调度和同槽位尾重启；DVD 队列读取
  真实 ISO，保留排序、扇区补齐和原等待逻辑。
- 存储卡提示文字、窗口和原 TIM2 光标已实际绘制。无卡和空目录分支执行原 Control、
  按键映射、有限真实音效及正常资源释放；完整存档读写尚未实现。
- 一次隐藏、静音的Windows原生合成“新游戏”运行完成489个tick，进入
  R100／Main state4；完成玩家更新1次、子模型更新1次和原物件更新73次，
  其中包括全部69个冷启动OMS物件及来自3EB的4个运行时教程物件。启用硬件
  静音音频、片头解码和完整独立ADX对照。合成按键不代表玩家实测。
- 存储卡 ADPCM 音效通过真实 XAudio2 处理；片头通过原生 FFmpeg 进程及 Windows
  音频设备播放。数值参考策略有明确范围，不声称完整音频或 PS2 实机位精确。
- 原系统、HUD、房间、相机和内存池初始化共用真实原生状态；描述解析不代表模型、
  动画、战斗或游戏场景已经完成。
- 首房间实际创建156个静态对象和全部69个冷启动OMS物件。玩家与子模型
  共用原版部件、节点、弹性骨骼和绑定；玩家初始化执行初始动作绑定、骨骼采样、
  地面IK与矩阵回写，随后完成出生和摄像机绑定。来自3EB的4个运行时教程
  物件使用原工厂和注册表。首个原始FX调用0x30F228完成，并创建4个环境发射器
  （bank3；ESP 活动槽为 0）。这不代表连续动画或玩法验收。
- 启用硬件静音音频、片头解码和完整独立ADX对照后，288个C模块通过318项检查，
  在固定源码副本验证。受限 GPU 像素与独立 CPU 方程一致；绘制保留原 OT 的层级、
  深度、同桶后进先出和上一帧提交顺序。
- 同一DLL中的两个RNG对10272个种子完成20544次状态／浮点位精确比较，依据实际指令的受限模型验证；
  这不是 PS2 实机验证。上游匹配比例也不是本项目的移植完成比例。

### 构建与运行

按上方英文段落的准备、构建和运行命令操作，替换为自己的 ISO、原生存档目录及
FFmpeg 路径。公共仓库不含游戏程序/资源、BIOS、私有 SDK、工具二进制或密钥。
Windows 窗口目标还需要 Windows SDK 的 x64 图形库、D3D11 硬件和音频设备。

窗口默认隐藏，`--show` 才显示；`--muted` 仅静音本程序，不改系统音量。原生存档
目录不存在时走真实无卡分支，打开时不会自动创建。默认键位为 Z=Cross、X=Circle、
方向键、Enter=Start、Escape=Triangle；确认/取消仍由原版 ELF 映射表决定。

当前以退出码 **4** 报告下一个尚未接通的原始调用，并非游戏完成。独立 `startup-qa` 目标才
包含合成输入选项 `--qa-confirm` 和 `--qa-new-game`；生产目标没有该选项。真实图形、音频或视频消费失败
会明确停止，不用空回调假装完成。

有用文件保存在 `E:\godhand_windows`，构建、缓存及大日志保存在
`G:\middle_thing\godhand_windows`。原镜像始终只读，原始反汇编等私有恢复证据及外部工具二进制不进 Git。
自制源码核验工具见 [证据工具说明](docs/EVIDENCE_TOOLS.md)。
独立 [PS2 移植 skill](skill/ps2-source-port/SKILL.md) 随开发维护。

原加密标题 BGM 已逐样本匹配独立 vgmstream 输出；真实资源测试也覆盖附属 ADPCM
音效与原随机音高。世界绘制保留原共享 OT 顺序、材质和打包坐标，完整 16 位 XY 覆盖
通过独立逐像素检查。原行走／跑步状态及 PL00 动画根位移已通过分项测试，平台模拟量
输入仍在接线。世界层已在开场遮罩之前验证；尚未进行真人游玩或可玩版本验收。

接下来继续原物件状态，推进至淡入结束，接通物理输入，并验证连续动画、战斗、
场景切换和存读档。本地提交保留回退，大节点才推送。当前状态见 [进展数据](docs/progress.json)。
