# God Hand case evidence

This is one modified title/release, not a universal PS2 specification. Current
progress/authorization belongs in [resume.md](resume.md).

## Baseline and upstream

Project: E:/godhand_windows. User ISO: God Hand (USA)(Uncensor Patch v1.0).iso.
Serial SLUS-21503, NTSC-U 1.00.

- Actual patched ELF SHA256: 3192ce89412d028a428970d3fcf58a56514878810e7e0608f2684966b10a3b95
- Upstream retail ELF SHA256: 1742f95bef65bdb2aa57b7a77df4ac7619a092e9646e1ea325bc32ec8a64f3cd
- Actual loaded-image SHA1: db2366e2e1cac197694a05d4df937eaf88fa43db
- Upstream loaded-image SHA1: 2a81167afa727a9e4293803a8139192e87cfdf17

Loaded images differ: do not claim only unloaded metadata changed. Reports:
analysis/disc-baseline.json and analysis/patched-baseline-comparison.json.

Upstream https://github.com/LucasPicoli/god-hand-decomp is pinned at
eaae18d0dfab1f9a0df276ae5ca1581ad326faf3 in vendor/god-hand-decomp. Preserve
its MIT reconstruction notice. It targets matching PS2 output, not a native
port. Its committed approximately 56.5% matched code bytes is not this project's
native completion percentage. Fully linked metrics can include assembly. The
large 0x0021A538 player dispatcher has guarded unverified C and an assembly
default at the audited commit; do not call it completely restored.

## Numerical correction

RNG 0x0031CF78 and 0x0031CF30 share word 0x00747B14, multiply by 0x10DCD
modulo 2^32 and retain zero as an absorbing state. Raw result scales the
converted unsigned word by 2^-31. An initial nearest-even conversion assumption
was rejected after actual instruction and PCSX2 EE ChopZero review. Local
integer-bit encoding implements chop conversion without changing host FEnv.
This applies to these functions, not all PS2 operations or games.

analysis/fpu-rounding-evidence.json cites official PCSX2 commit
0ac53afdc72dee599b422bb30a1d683f98e74625. It is emulator implementation
evidence, not Sony documentation or a PS2 hardware capture. The restricted
actual-instruction oracle checks state/float bits; it does not establish full
R5900 or whole-game correctness. See analysis/rng-differential.json.

## Startup and overlays

0x0030B298 initializes a 0x620-byte block at 0x007474A0. The native slice
preserves offsets and requires all platform callbacks; callback tests are not
real graphics/data/audio/IOP backends. 0x0030B728 subsystem/task initialization
and the 0x0030AC40 main-loop candidate remain separate dependency work.
Actual local Ghidra evidence is scoped in analysis/ghidra-startup.json.

The current case reports 1445 AFS/FST entries and 81 SNR2 modules. These counts
are observations for this baseline, not expected values for other games.
Matching overlay hashes establish content identity only. Indexing/parsing does
not execute gameplay. r207 is a small first candidate: consult current
analysis/r207-source-evidence.json and source before describing it as runnable.
The skill contains no game bytes or decompiler listings.

## Dynamic call and graphics corrections

The native0030D358 task now reaches the next task request002AE8A8 with real
bank/DVD/font/menu/ID work. This is cMc_taskBody, not a menu task. Its literal
jal0 at002AE8E8 is an unresolved SNR2 import: main-module relocation type4,
symbol index048D, name Main__7cMcBoot, supplied by mcport.rel export+0280.
Do not inherit upstream placeholder/no-op guesses for this call. The task's
002D5958 continuation uses002D5990 to set state2 and exit the current thread;
it must restart the same native slot, not return and mark it empty.

USA font MST uploads64x256 CT32 image bytes but samples them as512x256 PSMT4.
The paired-code draw consumer002A53A8 selects CLUT base1CF0/1CF1 by code parity;
it is not an unverified CSA choice. Cell=low12(code)>>1; glyph metric left/right
fields crop that cell. Native glyph1 is the original26x32 triangle button.
See analysis/font-texture-evidence.json for source addresses, independent
coordinate-bit equations and pinned primary implementation research. Final
screen transforms, tint and blend are separate from extracting glyph pixels.

Sound PrepareDecode naming was misleading: startup first prepares external AFS
directories, not bulk ADX PCM. Owned native bank commit and real AFD→AFS mapping
can satisfy startup table readiness without inventing an IOP reply. ADX0408 is
encrypted type8; no key/decoder is assumed. Voices and output remain separate.
