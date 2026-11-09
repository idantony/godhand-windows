# God Hand native core: RNG

Source: https://github.com/LucasPicoli/god-hand-decomp/tree/eaae18d0dfab1f9a0df276ae5ca1581ad326faf3

The upstream authored reconstruction is MIT licensed; its full copyright and
permission notice accompanies this module in `LICENSE.upstream`. This is
reconstructed source, not recovered Capcom original source. Game rights remain
with their respective owners.

| Native API | Original EE function | Upstream source |
| --- | --- | --- |
| `gh_rng_0031cf78` | `0x0031CF78` | `src/cod/0031cf78.c` |
| `gh_rng_0031cf30` | `0x0031CF30` | `src/cod/0031cf30.c` |

Both functions share the 32-bit global `D_00747B14` at EE virtual address
`0x00747B14`. The native caller owns that word explicitly. No seed policy,
thread synchronization, hardware emulation, or gameplay changes are introduced.
In particular, zero is a valid absorbing state; the wrapper then returns 1.
The raw routine scales by 2^-31, not 2^-32; do not normalize it to a conventional
random number range. The source's signed overflowing multiply is expressed as
unsigned modulo-2^32 multiplication.

Tests establish explicit chop binary32 source semantics, independently of the
host rounding environment. They are mathematical vectors, not PS2 captures.
The initial implementation assumed nearest-even. Review of the actual ELF
and PCSX2 implementation invalidated that assumption; it was replaced with
integer bit encoding of the EE chop conversion. The multiplier and state
transition remain unchanged. The output range is [0, 2), maximum raw bits
0x3fffffff; zero remains zero. Compared with ideal v / 2^31, the absolute
error is strictly below 2^-23 and the rounded result never exceeds the ideal.

Actual patched ELF SHA256:
`3192ce89412d028a428970d3fcf58a56514878810e7e0608f2684966b10a3b95`.
`tests/core/rng_elf_evidence.json` records its two RNG instruction ranges.
`analysis/fpu-rounding-evidence.json` records the aligned .text CTC1 scan:
only 0x00100118, writing $zero to FCR31. That zero does not imply generic
MIPS nearest rounding on this PS2 implementation. The high-bit unsigned
conversion path shifts right one, ORs the original low bit, converts a
signed integer, and doubles; the low-high-bit path converts directly. The
final multiply by 2^-32 and doubling are exact for these finite normal values.

Primary emulator evidence, pinned PCSX2 commit
`0ac53afdc72dee599b422bb30a1d683f98e74625`:

- https://github.com/PCSX2/pcsx2/blob/0ac53afdc72dee599b422bb30a1d683f98e74625/pcsx2/Pcsx2Config.cpp
  defines default EE FPU rounding as ChopZero.
- https://github.com/PCSX2/pcsx2/blob/0ac53afdc72dee599b422bb30a1d683f98e74625/pcsx2/FPU.cpp
  CTC1 stores FCR31; CVT_S converts signed integer under the configured FPU mode.
- https://github.com/PCSX2/pcsx2/blob/0ac53afdc72dee599b422bb30a1d683f98e74625/pcsx2/x86/iFPU.cpp
  emits CVTSI2SS/CVTDQ2PS for CVT_S; CTC1 does not select host rounding.

This is emulator implementation evidence, not Sony documentation or PS2
hardware capture. It does not establish correctness of all EE FPU operations.
Before integration, compare the original two functions against the native
module by setting/reading 0x00747B14 and capturing f0 raw bits. Validate the
unsigned conversion path with zero, one, high-bit words, conversion boundaries,
overflow cases and long sequences. No epsilon should hide a mismatch.

Compile rng.c as C11 or newer (no fast-math), include this directory, and link
`tests/core/test_rng.c` against it. Tests run the same vectors in every available
host rounding mode and ensure the API leaves that mode unchanged. Native build
and execution status is recorded separately by the parent build task. No
interpreter or emulator is required by the native library.
