# God Hand — Windows Source Port / Windows 源码移植

[English](#english) · [简体中文](#简体中文)

## English

Recover readable source for the original PS2 **God Hand**, preserve its gameplay
and assets, and replace platform dependencies with native Windows implementations.
This follows OpenGOAL's source recovery/platform adaptation method. God Hand was
not written in GOAL, so Jak's specialized decompiler is not directly applicable.

**Early development.** The original R100 opening plays all ten camera cuts
from the user's ISO and returns to the room. In a hidden, muted Windows run,
synthetic gamepad input moves Gene within roughly one unit of an original
enemy and starts the original Square attack. The fully visible room includes
successive attack frames. Gene's punch applies 3 damage to that enemy, whose
HP falls from 74 to 71 in presented scene frame 1,638. The run reaches 1,784
ticks and presents 1,643 scene frames, including title and room startup.
It submits 1,320,495 world packets with zero renderer, GPU, postprocess and
window errors.

The next original call reached is `0x00198CE0`. Continued combat, room
transitions and full save/load still need end-to-end verification.

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
- The opening runs with the original nine-actor roster, motion playback,
  model-backed particles, nested emitters and shared render queues. Final
  composites near the ninth cut show the original characters and street.
  The opening camera and event actor owners retire through the recovered quit path.
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
  creates four ambient emitters. Subsequent opening frames create original
  event effects, update their lifetimes and render sprites and framebuffer warp.
- The latest frozen source builds 365 C modules and passes 348 native checks.
  Eighteen checks that need extra tools or devices were skipped in this run.
  Earlier Windows verification exercised muted hardware audio, movie decoding
  and the full independent ADX comparison.
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
PL00 animation/root displacement pass focused tests. Windows QA exercises
native input mapping with synthetic gamepad samples. The opening's final
composites include the original fade, dialogue poses and camera changes.
The next scenario and combat paths remain under recovery.

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

The native profile directory represents card slot0. A missing directory takes
the original no-card path. Startup reads the original per-save header files
and validates their version. Full save restore and writing remain unfinished.
Keyboard defaults are Z=Cross, X=Circle, arrows, Enter=Start and
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
  --ffmpeg 'X:\tools\ffmpeg.exe' --hidden --muted --qa-new-game --max-ticks 2000
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

Next work continues the original post-opening Scenario, connects physical input,
and verifies combat, transitions, save/load and complete game progression.
Local commits retain rollback points. Public pushes mark substantial milestones.

## 简体中文

恢复《神之手》原游戏的可读源码，保留原玩法和资源，用原生 Windows 实现替换 PS2
平台依赖。采用 OpenGOAL 的源码恢复与平台移植方法；本作不是 GOAL 程序，不能直接
使用 Jak 专用反编译器。

**仍处于早期开发。** 已从用户 ISO 播完 R100 开场十个镜头并回到房间。隐藏、静音的
Windows 测试通过合成手柄输入让 Gene 接近原版敌人至约一个游戏距离单位，再按方块
触发原版攻击。房间淡入后，窗口连续呈现出拳动画，第 1638 个场景帧记录了命中和敌人 HP
从 74 降到 71。运行到 1784 ticks，呈现 1643 个场景帧，包含标题和房间启动帧。
累计提交 1320495 个世界绘制包，
渲染器、GPU、后处理和窗口错误均为 0。

运行随后到达原版调用 `0x00198CE0`。后续战斗、房间切换和完整存读档仍待端到端验证。

### 已验证的进展

- 从用户补丁镜像只读索引 1445 项资源和 81 个动态模块。模块哈希与固定上游一致；
  补丁主程序和加载映像与上游零售版不同，不能照搬其匹配结论。
- Windows fiber 实现原串行任务、嵌套 Scenario 调度和同槽位尾重启；DVD 队列读取
  真实 ISO，保留排序、扇区补齐和原等待逻辑。
- 存储卡提示文字、窗口和原 TIM2 光标已实际绘制。无卡和空目录分支执行原 Control、
  按键映射、有限真实音效及正常资源释放；完整存档读写尚未实现。
- 开场使用原版九个角色、动作、模型粒子、嵌套发射器和共享绘制队列。第九个镜头
  附近的最终合成帧可见原版角色和街道，开场相机和角色由恢复的清理流程退出。
- 存储卡 ADPCM 音效通过真实 XAudio2 处理；片头通过原生 FFmpeg 进程及 Windows
  音频设备播放。数值参考策略有明确范围，不声称完整音频或 PS2 实机位精确。
- 原系统、HUD、房间、相机和内存池初始化共用真实原生状态；描述解析不代表模型、
  动画、战斗或游戏场景已经完成。
- 首房间实际创建156个静态对象和全部69个冷启动OMS物件。玩家与子模型
  共用原版部件、节点、弹性骨骼和绑定；玩家初始化执行初始动作绑定、骨骼采样、
  地面IK与矩阵回写，随后完成出生和摄像机绑定。来自3EB的4个运行时教程
  物件使用原工厂和注册表。首个原始 FX 调用 0x30F228 创建 4 个环境发射器。
  后续片头帧创建原剧情特效，更新其生命周期，并绘制精灵和画面扭曲。
- 最新冻结源码编译 365 个 C 模块，通过 348 项原生检查，本次跳过 18 项需要额外工具或设备的检查。
  受限 GPU 像素与独立 CPU 方程一致。绘制保留原 OT 的层级、
  深度、同桶后进先出和上一帧提交顺序。
- 同一DLL中的两个RNG对10272个种子完成20544次状态／浮点位精确比较，依据实际指令的受限模型验证；
  这不是 PS2 实机验证。上游匹配比例也不是本项目的移植完成比例。

### 构建与运行

按上方英文段落的准备、构建和运行命令操作，替换为自己的 ISO、原生存档目录及
FFmpeg 路径。公共仓库不含游戏程序/资源、BIOS、私有 SDK、工具二进制或密钥。
Windows 窗口目标还需要 Windows SDK 的 x64 图形库、D3D11 硬件和音频设备。

窗口默认隐藏，`--show` 才显示；`--muted` 仅静音本程序，不改系统音量。原生存档
目录不存在时走原版无卡分支。启动可读取各存档头并校验原版版本。完整恢复和写入仍在开发。
默认键位为 Z=Cross、X=Circle、
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
输入仍在接线。开场最终合成画面已验证淡入、对话姿态和镜头切换，下一段剧情调度和
战斗仍在恢复。

接下来继续开场后的原剧情调度，接通物理输入，并验证战斗、场景切换、存读档及完整游戏
流程。本地提交保留回退，大节点才推送。当前状态见 [进展数据](docs/progress.json)。
