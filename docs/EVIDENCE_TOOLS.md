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
