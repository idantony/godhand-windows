# God Hand — Windows 源码移植

目标：恢复并验证《神之手》原游戏的可读代码，保留原玩法和资源，替换 PS2 平台依赖，构建独立 Windows 版本。采用 OpenGOAL 的源码恢复与平台移植方法；God Hand 原本不是 GOAL 程序，不能直接使用 Jak 专用反编译器。

**当前是源码恢复与原生模块阶段，还不能运行游戏。** 测试程序和 DLL 不是可玩的移植版，不是模拟器启动器。

## 文件位置

| 路径 | 用途 |
|---|---|
| 原始 `.iso` | 用户提供的补丁镜像，始终只读 |
| `src/` | 可读、可在 Windows 编译的移植源码 |
| `tests/` | 原函数证据和有意义的模块验证 |
| `tools/` | 可重复执行的分析、构建和验证入口 |
| `analysis/` | 版本、哈希、格式、反编译与验证记录 |
| `reference/` | 从自己的镜像提取的必要主程序、资源表和小样本 |
| `vendor/god-hand-decomp/` | 固定提交的社区源码恢复成果，保留原样与许可 |
| `.tools/` | 项目便携 Ghidra、Java、Zig/Clang 和开发用 Python 环境 |
| `G:\middle_thing\godhand_windows\` | 全部构建产物、缓存、临时文件和大型日志 |

## 已经验证的工作

- 镜像为美版 `SLUS-21503`、版本 1.00；实际 ISO、ELF 和各段哈希已记录。
- 补丁主程序及加载映像与上游零售版基准不同，不能照搬上游的 byte-matching 结论。
- 读取并索引 1445 项 AFS/FST 资源、81 个 SNR2 动态模块；81 个模块均与上游提交的哈希一致。
- 原生 C 资源层直接读取 ISO，无需解包整个镜像、Python 或 PS2 运行时；真实资源、路径查找和越界验证通过。
- 实际 R5900 ELF 已经由 PS2 专用 Ghidra 后端导入；13 个局部函数得到反编译证据，启动路径定位至 `0x0030AC40` 主循环候选。
- 两个 RNG 原函数 `0x0031CF78`、`0x0031CF30` 已移植为原生 C。保持模 2^32 状态演化，明确处理 EE 的 chop 舍入；不改变宿主浮点环境。
- 原生测试通过；对 10272 个种子、两个函数共 20544 次调用，实际 MIPS 指令的受限离线模型与 DLL 的状态及返回浮点位全部一致。该模型不是 PS2 实机，也不验证整台 EE。
- 首个小型房间模块恢复6个分配/释放、初始化与原本为空的更新函数；启动函数 `0x0030B298` 的控制流和固定宽度状态布局已移植。真实平台回调尚未接入。
- 房间、启动切片与 C++ 调用 C 接口的链接测试已纳入统一构建。

更多结果以 `analysis/native-verification.json`、`analysis/rng-differential.json` 和后续模块报告为准。
本地 `analysis/` 含原始恢复证据，不进入公共仓库；公开进展见 [docs/progress.json](docs/progress.json)。

## 可重复执行

PowerShell，工作目录 `E:\godhand_windows`：

```powershell
# 项目专用 C 编译器；下载地址及发布者 SHA-256 固定，缓存留在 G:。
.\tools\bootstrap-toolchain.ps1

# 编译并实际运行原生函数与原始资源测试。
python -B .\tools\build-native.py
python -B .\tools\verify_rng.py
python -B .\tools\export-progress.py

# 只读重做镜像与资源索引。
python -B .\tools\inspect_disc.py '.\God Hand (USA)(Uncensor Patch v1.0).iso'
python -B .\tools\inspect_assets.py '.\God Hand (USA)(Uncensor Patch v1.0).iso'

# PS2 专用 Ghidra 局部分析；项目和缓存写在 G:。
.\tools\run-ghidra.ps1
```

## 采用的技术与下一步

社区 [god-hand-decomp](https://github.com/LucasPicoli/god-hand-decomp) 的目标是用原 PS2 工具链重建相同的 PS2 二进制。我们复用其经过核查的符号、类型与源码，另外完成 Windows 平台层；其约 56.5% 匹配代码字节不等于本项目已移植的比例。

OpenGOAL 的借鉴点是[类型与逐函数恢复](https://opengoal.dev/docs/developing/decompiling/getting_started/)、[平台移植](https://opengoal.dev/docs/porting-info/porting_to_x86/)和[图形资源转换](https://opengoal.dev/docs/porting-info/drawable_and_tfrag/porting_tfrag/)。原游戏恢复代码及新增平台代码保留清楚的出处和验证范围。

接下来的关键工作是房间动态模块源码、启动和任务调度、原生图形与音频平台接入。必须继续验证真实游戏场景、转场、操作和存读档，才能交付可玩的 Windows 程序。

REA 已从 npm 官方 registry 和 GitHub 最新发行核查：本地 6.8.0 已是最新稳定版，Ghidra 与 MCP 专项检查通过。God Hand ELF 使用独立 Ghidra EE 后端，不改变已有 REA 配置。
