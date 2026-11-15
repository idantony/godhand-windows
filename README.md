# God Hand — Windows Source Port / Windows 源码移植

[English](#english) · [简体中文](#简体中文)

## English

Recover readable source for the original PS2 **God Hand**, preserve its gameplay
and assets, and replace platform dependencies with native Windows implementations.
This follows OpenGOAL's source recovery/platform adaptation method. God Hand was
not written in GOAL, so Jak's specialized decompiler is not directly applicable.

**Early development.** The Windows New Game replay plays all ten original
opening cuts and reaches two-way combat in R100. Four Triangle hits lower
the first enemy's HP from 74 to 26. The hidden, muted run presents 2,606
scene frames, including Gene's recovery prompt and an original grab with
its left-stick escape prompt. A second input replay triggers the paired
counter and registers collisions with two enemies, leaving them at 16 and
24 HP. A recorded pad route in a full Windows New Game replay defeats both
opening enemies with Gene at 1 HP. The original room save records both deaths
and Windows presents scene 3732. The replay schedules the original difficulty
confirmation to reproduce the startup timing. The same 2,125 gameplay inputs
match the native CPU replay. A longer Windows replay continues for 900
presented frames after victory, plays the original post-battle event and
enters the later grab-escape animation. Gene still has 1 HP.

In a Windows New Game replay, stick input reaches the first tutorial. Circle
opens the original panel and Cross dismisses it. The game presents the original
text and right-stick icon while the room remains visible.

A separate Windows title-load run restores an original-format test save and
presents R100. A controlled Windows run selects R003 at the title handoff,
presents all 28 original EV02 cuts over 2,785 frames, then follows the original
room exit to R105. After Gene's defeat, Cross confirms Continue in the original
Game Over menu. Windows presents the reloaded R105 with Gene at 100 HP. A
separate native CPU replay continues for 60 game frames after that reload.
Another actual-disc CPU replay restores the R100 checkpoint from Game Over and
registers movement and a Square attack after the reload. The normal R100 exit
after the opening encounter is still in development. An original controller
recording now reaches the AT10 area with Gene alive in a native CPU replay.

A separate Windows title-load replay uses an original-format R101 test save,
confirms the original chapter map and presents R101 at scene 461. It runs to
the planned 800-tick limit with 187 player updates and zero renderer errors.
Independent native CPU replays validate left-stick movement and a Square attack.
A longer Windows replay uses the original pad mappings to open the R101 gate
and walk into the corridor beyond it.

A controlled Windows replay begins with the R105 gate open in room save data
and Gene at the original exit. Native audio plays the gate sound, and the
original transition loads R002. Direct3D presents 3,390 consecutive R002
frames through all 24 shots of Event 04 and the original save prompt. The
replay selects Continue with the original pad input. A follow-up replay
presents the first 16 consecutive R102 frames with the original stage, Gene,
enemies and HUD. R102 movement and combat still need verification.

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
- The frozen regression build compiles 381 C modules and passes 371 native checks.
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
& '.\.work\build\windows-native\godhand-windows.exe' `
  --iso 'X:\your\God Hand.iso' --profile 'X:\your\native-profile' `
  --ffmpeg 'X:\tools\ffmpeg.exe' --show
```

The native profile directory represents card slot0. A missing directory takes
the original no-card path. Startup reads the original per-save header files
and validates their version. Title Load restores the verified R100 test profile.
In-game save creation and the full progression remain under development.
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
& '.\.work\build\windows-startup-qa\probe-native-startup.exe' `
  --iso 'X:\your\God Hand.iso' --profile 'X:\your\native-profile' `
  --ffmpeg 'X:\tools\ffmpeg.exe' --hidden --muted --qa-new-game --max-ticks 2000
python -B .\tools\build-native.py --hardware-audio --movie-codec 'X:\tools\ffmpeg.exe'
```

`--qa-confirm` and `--qa-new-game` exist only in the separate QA target. Real rendering/audio/video
consumers are required; unknown operations stop at explicit frontiers.

For a fast logic replay, build `--target startup-cpu-qa` and run
`probe-startup-cpu.exe` with the ISO path, FFmpeg path, absent profile path
and positive tick limit in that order. It drives the same title
and battle input, records completed packet frames, and reports the first
unsupported original operation. This target uses muted XAudio2 for sound
effects and collects drawing packets without presenting a window.

For a controlled R003 graphics replay, freeze the source and build the frozen
`startup-qa` target. Run `tools/run-startup-qa.py --r003-controlled` with that
`--snapshot`, the `--build-scratch` used by the build, and a fresh `--scratch`.
The probe uses the original R100 A38 destination at the title handoff. Its
report records presented R003 frames and EV02 cuts. Use `--capture-frame` to
select a scene serial and `--capture-count` to retain consecutive frames.
To inspect the same room's scheduler and drawing packets quickly, append
`--r003-controlled` to `probe-startup-cpu.exe` after the tick limit.

For the R102 presentation replay, run the `startup-qa` executable with
`--hidden --muted --max-ticks 7300 --qa-r102-capture` and add
`--qa-capture FILE --qa-capture-count 16`. Give this replay a fresh profile.
This option enables the test-only R105 gate and exit setup.

| Directory | Purpose |
|---|---|
| `src/`, `tests/` | Recovered source, native backends and meaningful checks |
| `tools/` | Input preparation, repeatable builds and inspection |
| `docs/` | Curated public progress |
| `skill/ps2-source-port/` | Independent reusable skill and resumable notes |
| `reference/`, `analysis/` | Private input/evidence, excluded from Git |
| `vendor/`, `.tools/` | Pinned upstream and local tools, excluded from Git |

Builds, caches and logs default to the ignored `.work/` directory in the
project. Scripts accept alternate scratch and compiler paths. Original game
data remains read-only. Attribution:
[NOTICE.md](NOTICE.md).

Next work continues the first battle and its exit, then verifies the subsequent
scene, in-game saving and full progression through the original game.
Local commits retain rollback points. Public pushes mark substantial milestones.

## 简体中文

恢复《神之手》原游戏的可读源码，保留原玩法和资源，用原生 Windows 实现替换 PS2
平台依赖。采用 OpenGOAL 的源码恢复与平台移植方法；本作不是 GOAL 程序，不能直接
使用 Jak 专用反编译器。

**仍处于早期开发。** Windows 新游戏回放播完原版开场十个镜头并进入 R100 双向战斗。
四次三角攻击命中，把第一个敌人的 HP 从 74 打到 26。隐藏、静音的实跑共呈现
2,606 个场景帧，包括 Gene 倒地后的恢复提示、原版抓技和左摇杆挣脱提示。另一轮输入回放触发了原版
挣脱反击，撞击后的两名敌人分别剩下 16 和 24 HP。Windows 新游戏回放用录制的手柄输入击败了
开场的两名敌人，Gene 剩 1 HP。原版房间存档记录两名敌人的死亡，Windows 呈现了第 3732 帧。
回放通过安排原版难度确认的时机复现启动流程。同样的 2,125 次玩家更新与无画面回放逐项对齐，
更长的一轮 Windows 回放在首战后继续呈现 900 帧，经过原版战后事件并进入随后一次抓技的
挣脱动画，Gene 仍剩 1 HP。

Windows 新游戏回放通过摇杆走到首个教程，按圆圈打开原版面板，再按叉键关闭。画面显示原版文字、
右摇杆图标和教程背后的场景。

独立的 Windows 标题读档实跑恢复了原版格式的测试档，并呈现 R100。受控实跑在标题交接处
选入 R003，呈现 EV02 全部 28 个镜头，共 2,785 帧，随后沿原版房间切换进入 R105。
Gene 战败后，按叉键在原版 Game Over 菜单中选择继续。Windows 呈现重载后的 R105，Gene
恢复到 100 HP。单独的无画面原盘回放在这次重载后又完成 60 帧。另一轮无画面原盘回放恢复了
R100 检查点，重载后接收移动和方块攻击。一轮原盘无画面回放通过录制的原版手柄输入走到 R100 的
AT10 区域，Gene 仍然存活。首战后从原版出口前往下一场景的流程仍在开发。

另一轮 Windows 回放从标题读取 R101 的原版格式测试档，经原版章节地图确认，在第 461 帧呈现
R101。运行到预定的 800 tick 上限，完成 187 次玩家更新，渲染无报错。独立的无画面回放也
验证了摇杆移动和方块攻击。更长的 Windows 回放使用原版手柄映射打开 R101 的门，并走进门后的走廊。

一轮受控的 Windows 回放从 R105 已开门的存档状态开始，并把 Gene 放在原版出口。原生音频
播放开门声，原版房间切换载入 R002。Direct3D 连续呈现了 R002 的 3,390 帧，完整播放 EV04 的
24 个镜头并显示原版保存界面。回放用原版按键选择继续。后续回放连续呈现了 R102 的前 16 帧，
显示原版场景、Gene、敌人与 HUD。R102 的移动与战斗仍待验证。

### 已验证的进展

- 从用户补丁镜像只读索引 1445 项资源和 81 个动态模块。模块哈希与固定上游一致；
  补丁主程序和加载映像与上游零售版不同，不能照搬其匹配结论。
- Windows fiber 实现原串行任务、嵌套 Scenario 调度和同槽位尾重启；DVD 队列读取
  真实 ISO，保留排序、扇区补齐和原等待逻辑。
- 存储卡提示文字、窗口和原 TIM2 光标已实际绘制。无卡和空目录分支执行原 Control、
  按键映射、有限真实音效和正常资源释放。原版格式的测试存档可从标题菜单恢复并进入 R100。
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
- 冻结回归构建编译 381 个 C 模块，通过 371 项原生检查，本次跳过 18 项需要额外工具或设备的检查。
  受限 GPU 像素与独立 CPU 方程一致。绘制保留原 OT 的层级、
  深度、同桶后进先出和上一帧提交顺序。
- 同一DLL中的两个RNG对10272个种子完成20544次状态／浮点位精确比较，依据实际指令的受限模型验证；
  这不是 PS2 实机验证。上游匹配比例也不是本项目的移植完成比例。

### 构建与运行

按上方英文段落的准备、构建和运行命令操作，替换为自己的 ISO、原生存档目录及
FFmpeg 路径。公共仓库不含游戏程序/资源、BIOS、私有 SDK、工具二进制或密钥。
Windows 窗口目标还需要 Windows SDK 的 x64 图形库、D3D11 硬件和音频设备。

窗口默认隐藏，`--show` 才显示；`--muted` 仅静音本程序，不改系统音量。原生存档
目录不存在时走原版无卡分支。标题读档已验证恢复 R100 测试档。游戏内存档和完整流程仍在开发。
默认键位为 Z=Cross、X=Circle、
方向键、Enter=Start、Escape=Triangle；确认/取消仍由原版 ELF 映射表决定。

当前以退出码 **4** 报告下一个尚未接通的原始调用，并非游戏完成。独立 `startup-qa` 目标才
包含合成输入选项 `--qa-confirm` 和 `--qa-new-game`；生产目标没有该选项。真实图形、音频或视频消费失败
会明确停止，不用空回调假装完成。

快速逻辑回放使用 `--target startup-cpu-qa`。运行 `probe-startup-cpu.exe` 时依次传入 ISO、
FFmpeg、一个不存在的存档目录和正整数 tick 上限。该工具复用标题和战斗的测试输入，
记录完成的绘制指令帧，并报告第一个尚未接通的原始调用。工具以静音 XAudio2 处理音效，
收集绘制指令时不呈现窗口。

单独验证 R003 画面时，先冻结源码并构建该快照的 `startup-qa` 目标。用
`tools/run-startup-qa.py --r003-controlled` 指定 `--snapshot`、构建时的
`--build-scratch` 和新的 `--scratch`。回放在标题交接处使用原版 R100 A38
目的地，报告实际呈现的 R003 帧和 EV02 镜头。用 `--capture-frame` 指定画面帧号，
用 `--capture-count` 保存连续帧。
需要快速查看同一房间的调度和绘制指令时，在 `probe-startup-cpu.exe` 的 tick 上限后
追加 `--r003-controlled`。

复现 R102 画面时，给 `startup-qa` 可执行程序传入
`--hidden --muted --max-ticks 7300 --qa-r102-capture`，并用
`--qa-capture FILE --qa-capture-count 16` 保存画面。请使用新的存档目录。
该选项启用测试用的 R105 门与出口起始状态。

构建、缓存和日志默认保存在项目内由 Git 忽略的 `.work/` 目录。原镜像始终只读，
原始反汇编等私有恢复证据及外部工具二进制保存在本地。
自制源码核验工具见 [证据工具说明](docs/EVIDENCE_TOOLS.md)。
独立 [PS2 移植 skill](skill/ps2-source-port/SKILL.md) 随开发维护。

原加密标题 BGM 已逐样本匹配独立 vgmstream 输出；真实资源测试也覆盖附属 ADPCM
音效与原随机音高。世界绘制保留原共享 OT 顺序、材质和打包坐标，完整 16 位 XY 覆盖
通过独立逐像素检查。原行走／跑步状态及 PL00 动画根位移已通过分项测试，平台模拟量
输入仍在接线。开场最终合成画面已验证淡入、对话姿态和镜头切换，下一段剧情调度和
战斗仍在恢复。

接下来继续首战和开门流程，验证后续场景、游戏内存档和完整游戏流程。
本地提交保留回退，大节点才推送。当前状态见 [进展数据](docs/progress.json)。
