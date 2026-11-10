# God Hand — Windows Source Port / Windows 源码移植

[English](#english) · [简体中文](#简体中文)

## English

Recover readable source for the original PS2 **God Hand**, preserve its gameplay
and assets, and replace platform dependencies with native Windows implementations.
This follows OpenGOAL's source recovery/platform adaptation method. God Hand was
not written in GOAL, so Jak's specialized decompiler is not directly applicable.

**Early development. No playable original game scene has been verified.**
A native Windows execution connects the original memory-card interface, Clover
movie, title scene and New Game transition into room R100. A bounded synthetic
run presents 6 MC, 90 movie and 369 title/menu frames, completes original
player initialization, spawn/camera binding, all 69 first-room object
initializers and four runtime tutorial objects. R100 Init completes and Main
state4 now executes NearCheck, world lighting and positive HitArea processing,
then enters the player’s first update. Original idle selection, a 10-frame blend
and the first bone sample run; the next unresolved call is input/interaction
`0x00123938`. It begins at startup task `0x0030D358`; earlier boot, the first
world frame and gameplay remain unfinished.

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
- One hidden, muted native Windows QA run waits for video EOF and real audio
  completion, executes Title/New Game and reaches the player’s first update in R100/GameMain state4.
  Its keys are an explicitly labeled synthetic fixture; no human playtest is claimed.
- Limited MC ADPCM voices use real XAudio2. Movie0 uses an explicit native
  FFmpeg codec process and Windows audio. Reference DSP/GS numerical profiles
  are documented; they are not PS2 hardware-equivalence claims or complete audio.
- Original case0 initialization, HUD setup, room state, camera reset and room
  allocation operate on shared native owners. Asset descriptors are distinct
  from completed model/animation rendering or gameplay.
- Actual first-room construction creates 156 static objects and all 69 OMS
  objects. Parent PL00 and child PL01 share original parts/nodes, child springs
  and attachment. Original player initialization executes initial motion binding,
  bone sampling, terrain IK and matrix recovery, followed by spawn and camera
  binding. Four runtime tutorial objects also use the same original factory
  and registries. This is not a continuous animation or gameplay acceptance test.
- 221 C modules pass 237 checks with hardware/codec checks and an independent
  ADX reference WAV enabled.
  The earlier foundation also passes in an independent clean source snapshot.
  Restricted GPU pixels match separate CPU equations. The recovered OT traversal
  preserves bank order, bucket depth, LIFO packet groups and previous-frame presentation.
- Two RNG routines pass 20544 exact state/float-bit comparisons across 10272
  seeds against a restricted actual-instruction model, not console traces.

Native encrypted title BGM decoding now matches every sample from independent
vgmstream output. The original warning, logos and title screen now execute in
the integrated native scene. New Game now loads the first room and advances
through player initialization, all first-room objects and tutorial initialization
through Main state4 into player idle motion and its input/interaction call
`0x00123938`.
The actual run submits zero
world packets. World rendering, animation, combat, complete audio and save/load
still need implementation.

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

Next: R100 scenario/Event scripts, the first original world frame, continuous
animation, combat, transitions
and save/load acceptance. Local commits retain rollback points; public pushes
mark substantial milestones.

## 简体中文

恢复《神之手》原游戏的可读源码，保留原玩法和资源，用原生 Windows 实现替换 PS2
平台依赖。采用 OpenGOAL 的源码恢复与平台移植方法；本作不是 GOAL 程序，不能直接
使用 Jak 专用反编译器。

**仍处于早期开发，尚未验证可玩的原游戏场景。** 同一个 Windows 原生程序已串联
原版存储卡界面、Clover 片头、标题场景和“新游戏”到 R100 的转换。一次有界合成输入
验收显示 6 帧存储卡、90 帧片头和 369 帧标题／菜单，完成玩家初始化、出生位置／
摄像机绑定、全部 69 个首关物件及 4 个运行时教程物件初始化。R100 Init 完成后，
Main state4 已执行 NearCheck、世界光照与正碰撞处理，进入玩家第一次更新；原待机
动作选择、10 帧过渡绑定和首次骨骼采样已运行，下一调用是输入／交互
`0x00123938`。执行从原启动任务 `0x0030D358` 开始，前置启动、首个实际
3D 场景画面和玩法仍未完成。

### 已验证的进展

- 从用户补丁镜像只读索引 1445 项资源和 81 个动态模块。模块哈希与固定上游一致；
  补丁主程序和加载映像与上游零售版不同，不能照搬其匹配结论。
- Windows fiber 实现原串行任务、嵌套 Scenario 调度和同槽位尾重启；DVD 队列读取
  真实 ISO，保留排序、扇区补齐和原等待逻辑。
- 存储卡提示文字、窗口和原 TIM2 光标已实际绘制。无卡和空目录分支执行原 Control、
  按键映射、有限真实音效及正常资源释放；完整存档读写尚未实现。
- 一次隐藏、静音的 Windows 原生验收显示了 6 帧存储卡界面和全部 90 帧 Clover
  片头，确实等待视频结束及音频设备完成，再执行标题／新游戏流程进入 R100、Main state4 的玩家首次更新。
  确认按键由明确标注的合成测试输入提供，不冒充玩家实测。
- 存储卡 ADPCM 音效通过真实 XAudio2 处理；片头通过原生 FFmpeg 进程及 Windows
  音频设备播放。数值参考策略有明确范围，不声称完整音频或 PS2 实机位精确。
- 原系统、HUD、房间、相机和内存池初始化共用真实原生状态；描述解析不代表模型、
  动画、战斗或游戏场景已经完成。
- 首房间实际创建 156 个静态对象和全部 69 个 OMS 物件。玩家与子模型共用原版部件、
  节点、弹性骨骼和绑定；原玩家初始化执行初始动作绑定、骨骼采样、地面 IK 与矩阵回写，
  随后完成出生位置和摄像机绑定；4 个运行时教程物件共用原工厂和对象列表。
  尚未提交世界绘制包，未验证连续动画或可玩场景。
- 启用硬件/解码器检查后，221 个 C 模块通过 237 项检查（包含独立 ADX 参考 WAV），
  在固定源码副本验证。受限 GPU 像素与独立 CPU 方程一致；绘制保留原 OT 的层级、
  深度、同桶后进先出和上一帧提交顺序。
- 两个 RNG 对 10272 个种子完成 20544 次精确比较，依据实际指令的受限模型验证；
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

原加密标题 BGM 已逐样本匹配独立 vgmstream 输出；原警告、Logo 和标题画面已接入
完整的当前原生启动流程。合成“新游戏”输入已进入首关并执行原玩家／物件初始化，
当前已执行玩家首次待机动作绑定和采样，停在输入／交互 `0x00123938`。
首个世界画面、连续动画、战斗、完整音频和存档读写仍需继续。

接下来继续首关 Scenario／Event 脚本、首个原游戏画面、连续动画、战斗、场景切换与
存档验收。本地提交保留回退，大节点才推送。当前状态见 [进展数据](docs/progress.json)。
