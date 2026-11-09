# God Hand — Windows Source Port / Windows 源码移植

[English](#english) · [简体中文](#简体中文)

## English

Recover readable source for the original PS2 **God Hand**, preserve its gameplay
and assets, and replace platform dependencies with native Windows implementations.
This follows OpenGOAL's source recovery and platform adaptation method. God Hand
was not written in GOAL, so Jak's specialized decompiler is not directly usable.

**Early development; no playable game scene, controls or audible output yet.**
The developer startup host executes a recovered task with real native backends.
It stops explicitly when the next original task is unavailable.

### Verified progress

- Indexed 1445 resources and 81 dynamic modules from the user's patched NTSC-U
  `SLUS-21503` disc. All 81 module hashes match the pinned upstream manifest.
  The boot ELF and loaded image differ from the upstream retail baseline.
- Native serial tasks use Windows fibers to preserve their C stacks across the
  original sleep/exit handshake. DVD jobs read actual ISO sectors, preserve
  priority ordering and implement the original extra wait after completion.
- The actual `0x0030D358` startup task now loads sound banks and core/language
  bundles, then initializes original font, menu and ID state. Its normal path
  enters `0x002AE8A8`, verifies the original MC module and initializes67 real UI
  entries and message state. It currently stops at mcport's Control routine.
  This starts at that task, not at the game's entry point or full boot sequence.
- Sound startup commits owned program/sample tables and real AFD→AFS mappings;
  ready flags are `0x7F/0x7F/0x0F`. Voice execution and audio output are pending.
- Original font packets now decode into native atlas/glyph RGBA, including the
  original odd/even CLUT selection. Final screen drawing and blending are pending.
- Bounded native asset views cover core bundles, indexed TIM2 textures, SCR mesh
  descriptors and SES banks. A descriptor parser is not model/animation rendering.
- Nine r207 routines and a boot dependency slice are recovered. Two RNG routines
  pass 20544 exact state/float-bit comparisons across 10272 seeds against a
  restricted actual-instruction model. This is not PS2 hardware validation.
- Original portrait texture decoding and offscreen RTX GPU upload/readback pass.
  This is asset QA, not a rendered game scene.

Current checks and boundaries: [docs/progress.json](docs/progress.json).
Milestones: [docs/PROGRESS.md](docs/PROGRESS.md).
Upstream PS2 matching percentages do not measure this port's progress.

### Prepare and verify

The repository contains authored source, tests and tools. It includes no ISO,
game binary/assets, BIOS, private SDK or credentials. Use your own game data.

```powershell
.\tools\bootstrap-toolchain.ps1
python -B .\tools\prepare-inputs.py --iso 'X:\your\God Hand.iso'
python -B .\tools\build-native.py
python -B .\tools\verify_rng.py
python -B .\tools\export-progress.py
python -B .\tools\check-public-tree.py
```

The bootstrap verifies the compiler's published SHA-256. Preparation pins the
upstream checkout and reads the ISO without bulk extraction. The integrated
build records source fingerprints and executes native module/integration checks.

The development host is built in the external scratch tree:

```powershell
& 'G:\middle_thing\godhand_windows\build\native\run-startup.exe' 'X:\your\God Hand.iso'
```

It currently reports an expected next-entry frontier with exit code **2**;
that is an incomplete execution slice, not game startup success. Its ticks are
scheduler visits, not timed video frames. Bank loading uses synchronous host I/O.

| Directory | Purpose |
|---|---|
| `src/`, `tests/` | Native source and module/integration checks |
| `tools/` | Repeatable input preparation, inspection and verification |
| `docs/` | Curated public progress |
| `skill/ps2-source-port/` | Independent reusable skill and resumable project notes |
| `reference/`, `analysis/` | Private input samples and recovery evidence; excluded from Git |
| `vendor/`, `.tools/` | Pinned upstream source and project tools; excluded from Git |

This workspace keeps useful files under `E:\godhand_windows` and generated
build/cache/log files under `G:\middle_thing\godhand_windows`. Build scripts accept
external scratch/compiler options. The original disc is always read-only.
See [NOTICE.md](NOTICE.md) for attribution.

### Next work

Connect memory-card Control/drawing and GameMain state1, then the earlier
boot and full main loop, and implement rendering, voices/audio output, input,
model/animation loading and save services. Verify real scenes and progression.
Local commits retain rollback points; public pushes mark substantial milestones.

## 简体中文

恢复并验证《神之手》原游戏的可读源码，保留原玩法和资源，替换 PS2 平台依赖，
构建独立 Windows 版本。采用 OpenGOAL 的源码恢复与平台移植方法；本作原本不是
GOAL 程序，不能直接使用 Jak 专用反编译器。

**目前还没有可玩的游戏场景、操作或声音输出。** 开发用原生启动宿主已能运行一个
恢复后的原任务，并在缺少下一原任务时明确停止。

### 已验证的进展

- 从用户提供的美版补丁镜像索引 1445 项资源和 81 个动态模块；81 个模块哈希均与
  固定上游提交相同。补丁主程序和加载映像与上游零售版不同，不能照搬匹配结论。
- Windows fiber 保存真实 C 调用栈，实现原任务的启动、睡眠和退出交接。DVD 队列
  读取真实 ISO 扇区，保留优先级排序及完成后的额外一次等待。
- 原 `0x0030D358` 任务已串联声音 bank 加载、核心包/语言包读取、字体绑定、菜单表
  和 ID 初始化。默认路径已进入 `0x002AE8A8`，核验原存档模块并初始化67个真实 UI
  对象和消息环境，目前停在模块 Control 循环。
  这段执行从该任务开始，并未运行完整游戏入口和前置启动。
- 声音启动持有真实程序/样本表，建立原 AFD→AFS 映射；ready 标志为
  `0x7F/0x7F/0x0F`。事件执行、voices 和声音输出尚未完成。
- 原字体图集与单字形已能解码为原生 RGBA，保留原来的奇偶 CLUT 选择。
  最终屏幕绘字、着色与混合仍需接入。
- 原生资源层包含核心包、受限 TIM2 纹理、SCR 网格描述和 SES bank。
  解析描述信息不代表已经能绘制模型或动画。
- 已恢复 r207 的九个小型函数和启动依赖切片。两个 RNG 原函数对 10272 个种子、
  共 20544 次调用，与实际指令受限模型的状态及浮点位完全一致；这不是 PS2 实机验证。
- 原 UI 头像纹理已成功解码，并通过 RTX GPU 离屏上传/读回验证；这是资源审核，
  还不是原游戏场景渲染。

当前验证与限制见 [docs/progress.json](docs/progress.json)，里程碑见
[docs/PROGRESS.md](docs/PROGRESS.md)。上游 PS2 字节匹配比例不是 Windows 移植完成比例。

### 使用自己的镜像验证

公共仓库不包含游戏镜像、程序/资源、BIOS、私有 SDK 或密钥。
执行上方英文段落的准备和构建命令即可；ISO 路径换成自己的文件。
原始镜像始终只读，不会整盘解包。构建会记录输入哈希并运行模块和集成验证。

`run-startup.exe` 当前以退出码 **2** 报告下一入口尚未实现。这是明确的执行边界，
不代表游戏启动成功。输出的 tick 是调度访问次数，不是视频帧；声音 bank 使用同步
宿主读取来替代原异步时序。

源码和有用文件保存在 `E:\godhand_windows`，构建、缓存和临时日志保存在
`G:\middle_thing\godhand_windows`。原始提取样本、私有恢复证据、上游源码和工具不进 Git。
独立的 [PS2 移植 skill](skill/ps2-source-port/SKILL.md) 随开发维护，保留可恢复的项目状态。

后续继续接入存档 Control/绘制和 GameMain 的case1，再接前置启动与完整主循环、渲染、声音、输入、
模型/动画和保存服务，再验证真实场景与游戏流程。本地小提交保留回退点，大节点再推送。
