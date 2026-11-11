# Evidence tools / 证据工具

These project tools support readable source recovery. They do not generate a
finished port or execute guest instructions in the production runtime.

- `tools/run-ghidra.ps1` exports explicit function probes with the installed
  Ghidra/EmotionEngine language. `-MaxInstructions` accepts 1..65536 and defaults
  to 2048. Reports include the limit, captured count and `disassemblyTruncated`.
  Requested and expanded addresses are normalized to avoid duplicate entries.
- `tools/function-context.py` verifies the report's ELF identity, raw instruction
  bytes/words, function ranges and direct-call targets. Version2 summarizes
  verified static memory operands as register + signed offset + nominal width.
  Register operands alone do not establish class fields, aliases or live values.
  Optional SNR2/REL metadata can identify imports; unresolved calls stay unknown.
- `tools/freeze-source.py` records immutable authored source and checked worker
  bundles. Build reports fingerprint the exact inputs; staged source can come
  from that verified copy while other workers continue editing.
  Worker receipts with absolute paths use their `relative` names and must
  supply each `supersedes` fingerprint or explicitly declare a new file.
- `tools/rebase-source-overlay.py` moves a pinned text overlay to a newer frozen
  source snapshot. It verifies both receipts and prerequisites, merges disjoint
  edits, and writes a fresh bundle to project scratch. Absolute worker receipts
  also check their relative source and predecessor paths. Overlapping edits fail
  before the new bundle is published.
- `tools/record-startup-frontier.py` verifies a completed Windows QA process
  against its source and executable receipts, records the observed original
  stop or native rendering error, and fingerprints its logs and optional raw
  capture. Its output records an unfinished observation.
  Use `--title-load` for a title Load Game QA cohort. It verifies the isolated
  profile path and records the card and UI phases, slot mask and presents.
  `tools/prepare-title-load-qa.py` creates a fresh synthetic slot3 with a valid
  original timestamp and checksum in that scratch profile. Pass
  `--build-scratch` to both the launcher and the recorder when a new execution
  cohort reuses a verified executable from an earlier scratch. The source
  snapshot, compiled executable and runner still have matching fingerprints.
  `inspect-title-graphics.exe ISO --texture 1 49` reads a language texture's
  TIM2 dimensions, palette format and TEX0 even before the renderer admits it.
  Windows builds and native tests share compiled C objects across frozen
  source copies. A changed source or reachable project header changes that
  unit's cache key. Cached object bytes are checked before linking. Both
  builders accept `--object-cache` to select a shared cache and
  `--no-object-cache` for a cold build. Native test executables link against
  one archive of compiled modules. `--focus game-save-roundtrip` exercises
  the real Door map and slot write with an isolated card profile.
  When attached audio fails at the stopping original address, the proof
  records its bank, program, part, flags and delay.
  The QA launcher rejects a mismatch with the snapshot's launcher before it
  reserves outputs, since the execution proof checks those exact bytes.
  `--capture-series` verifies each logged 512×448 world/composite pair against
  its process hash and counts nonblack RGB pixels independently of alpha.
  Compare both surfaces and the logged Fade color when diagnosing a black frame.
  Builds with capture presentation tracking identify which prepared frames
  were presented by the window before the execution stopped.
  `--require-presented-player-damage` checks an original player Take record
  and the same target's HP decrease on successive visible presented frames.
  The QA `QA_SCENARIO_TASK` marker identifies a failed nested task slot and
  Scenario substage when the host reports an original script frontier.
  `GH_ACTOR_FRONTIER` ties the original draw stop to a separate actor handle,
  class, Draw substage and raw root flags.
- Windows startup QA records the current 0202 actor's original placement and
  state bytes in the `QA_ENEMY_PLACEMENT` stderr record when available.
- `tools/build-native.py --focus game-enemy-started-pitch --focus game-om324-started-pitch`
  runs the original-disc voice regressions with a test-only recording transport.
  They verify the PCM and command sequence without opening an audio device.
- `tools/normalize-staged-eof.py` plans or removes reported terminal blank lines
  from Git's staged text only. Other whitespace errors stop it before the index
  changes. It preserves the working tree, frozen snapshots and executable modes.
- `tools/check-test-contracts.py` checks literal C entry-point `argc` assertions
  against the build manifest before compilation. The build runs this check too.
  Conditional, optional or unrecognized contracts remain explicitly unknown;
  the actual tests still validate their arguments and behavior.

Example using your own prepared input and an installed analysis toolchain:

```powershell
.\tools\run-ghidra.ps1 -ProjectName local_probe -Addresses @('00108AB0') -MaxInstructions 4096 -OutputJson 'E:\godhand_windows\analysis\player-private.json'
python -B tools/function-context.py --report analysis/player-private.json --elf reference/SLUS_215.03 --address 00108AB0 --output analysis/player-context-private.json
python -B tools/check-test-contracts.py
python -B tools/normalize-staged-eof.py
python -B tools/normalize-staged-eof.py --apply --report analysis/new-eof-receipt-private.json
```

Generated disassembly/decompiler reports stay in ignored `analysis/`; scratch
projects, caches and builds stay on G:. Public source includes no game bytes or
tool binaries. Byte verification is evidence provenance, not proof that inferred
C, complete control flow, numerical behavior or gameplay is correct. Separate
native tests and actual execution remain required.

这些工具服务于可读源码恢复，不会直接生成完整移植版。Ghidra 导出器明确记录大函数的
指令上限和截断状态；函数工具核对用户 ELF 的实际字节、调用／导入及静态访存操作。
“寄存器＋偏移”仍需结合调用者和生命周期才能解释为字段，未知项保留未知。
固定源码工具让多智能体继续工作时，验证仍对应同一批准确源码。原始反汇编报告留在
私有 analysis，临时项目／缓存／构建放 G 盘；公开仓库只包含自制源码。字节核对不能
代替原生测试、实际执行或可玩验收。

同类可复现问题第二次出现时，优先固化成检查或修复工具。行尾工具只处理暂存区中
报告的尾部空行；参数工具会在编译前拦住“测试需要一个文件却传了两个”的明确矛盾。
未知参数约定仍保留未知，两项工具均有回归及真实失败案例验证。


`run-ghidra.ps1` validates each new headless run before publishing its report.
Pass addresses as quoted hexadecimal strings: PowerShell can interpret an
unquoted value containing `e` as scientific notation. The wrapper uses a new G:
run directory, checks logs and the complete requested function set, and verifies
raw words against the same ELF. A process exit code of zero is insufficient.
Choose a fresh output filename; a failed or repeated probe never replaces an
existing report. `tests/tools/test_validate_ghidra_report.py` covers these failures.

Ghidra 包装工具现在会核对本轮日志、完整函数清单、输出生成时间和实际 ELF 指令字。
地址必须写成带引号的十六进制字符串，避免 PowerShell 把含 e 的地址解析为科学计数。
即使进程返回 0，Java 报错或缺少报告也会失败；已有报告始终保留，请使用新输出文件名。

`tools/inspect-actor-sound.c` audits core sound programs by default. Pass
`--ses se/em00.ses 0 1 2` after the ISO and report paths to inspect those
programs in an isolated SES registry. The report includes program fields,
sample offsets, ADSR and decoded frame counts. It writes JSON only and uses
no audio device. Registry bank 1 in this report is local to the audit.

声音检查工具可用 `--ses se/em00.ses 0 1 2` 核对指定音效表、样本偏移、ADSR 和解码
帧数。报告的 bank 1 是检查工具的临时编号，游戏运行时由原版音效表管理器分配编号。

`tools/export-startup-capture.py --proof RECEIPT --output IMAGE.png` converts
the latest nonblack, presented composite to RGB PNG after checking its recorded
capture hash. `--scene` selects a specific presented frame. Use `--layer world`
for the pre-UI surface and label that image accordingly when sharing it.

截图导出默认取最后一个已显示的非黑合成帧，并核对原始像素的哈希。指定 `--scene`
可导出某一已显示帧。`--layer world` 输出叠加 UI 和遮罩之前的场景层。

`tools/record-native-milestone.py` consolidates matching private evidence. It
checks source, artifact and report hashes, the executed Windows path, two RNG
reports and the actual hidden/muted QA result. A nonzero unfinished stop needs
an explicit matching `--expected-frontier`; it never becomes playable acceptance.
The output must be a new ignored `analysis/*-private.json`, and existing evidence
is not overwritten. It does not build, launch the game or publish progress.

```powershell
python -B tools/record-native-milestone.py --snapshot 'G:\middle_thing\godhand_windows\snapshots\room-transition-67' --scratch 'G:\middle_thing\godhand_windows\verification\room-transition-67' --output 'E:\godhand_windows\analysis\new-proof-private.json' --expected-frontier 00186928
```

验收汇总工具核对固定源码、二进制、报告与实际运行是否属于同一批；失败报告、混合
版本和覆盖旧证据都会被拒绝。未完成的停止必须明确指定预期地址；工具只记录事实，
不推算完成比例，不把开发测试通过当成可玩验收，也不会自动推送仓库。
