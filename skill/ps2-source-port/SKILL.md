---
name: ps2-source-port
description: Recover readable code from a PS2 game and migrate its original behavior to a native host platform using versioned binary evidence and function-level verification. Use for matching-decomp integration, PS2 ABI and platform replacement, or continuing a source port; not for gameplay remakes or packaging general-purpose emulators.
---

# PS2 source recovery and native porting

Deliver readable reconstructed game code plus a native platform implementation.
Preserve original behavior and assets. A restricted instruction interpreter or
recompiler can support development verification; it does not substitute for
source recovery or a finished native runtime when a source port was requested.
OpenGOAL's method does not make its game/language-specific decompiler applicable
to unrelated titles.

At entry, identify the requested game's project root and read its AGENTS/README
before choosing inputs or tools. For a new game, establish its own baseline and
independent resume; do not start from the God Hand resume. Current instructions
and scope (including a read-only request) override recorded historical authority.

For the active God Hand project, read [references/resume.md](references/resume.md)
for paths, authorization and current reports, then
[references/godhand-case.md](references/godhand-case.md) for validated exceptions.
Other games inherit neither its paths/counts/numerical assumptions nor public
publishing permission. Read each project's instructions before modifications.

## Establish the actual baseline

Inventory the user's own disc read-only. Record ISO hash, serial/version, ELF
hash, load segments and overlay hashes. Pin upstream decompilation commits and
retain licenses. Read setup scripts before execution; public links to private
SDK binaries do not establish permission to download/use them. Do not acquire
an unknown retail ISO/ELF to make a hash match.

When the user's ELF differs from upstream, hash the loaded image separately
from ELF metadata before attributing changes to a patch. A whole-image hash
cannot locate changed functions. Save function ranges/hashes and inspect actual
local instructions for each ported unit. Keep the actual binary as ground truth
and unknown facts explicit. Prefer primary documentation/implementation evidence;
distinguish emulator behavior from hardware measurements.

## Recover types and useful executable slices

Separate original symbols, overlay exports, inferred names and analyst labels.
Maintain function addresses, dependencies and confidence. Recover field offsets
from loads/stores and call sites. Use fixed-width fields, explicit padding and
size/offset assertions. Do not transplant PS2 long, bitfields, guest pointers or
calling conventions into a 64-bit host without explicit ABI adaptation. Resolve
guest addresses through an explicit mapping before native dereferences; verify
bounds and lifetime for any intentional fixed mapping.

Resolve dynamic-module relocations before classifying zero/unknown calls.
A literal jump target0 may be an unbound import patched by the original loader,
not logging or an empty function. Trace the relocation type, symbol index/name
and supplying module export. Do not skip it merely because upstream guessed a
placeholder name. Function-address-like source filenames also need declaration
and linker-section verification.

Recover a small algorithm/subsystem with state and callers, then extend toward
startup, scheduling and a real scene. Keep original behavior reconstruction
separate from platform replacements. Record assembly fallbacks, guarded
NON_MATCHING bodies and unverified C separately. A byte-identical linked ELF
can contain retail assembly: it is not a completed readable-C or native port.
Report byte/function denominators and their scope rather than a single vague
completion percentage.

Make platform boundaries meaningful operations with traceable source addresses.
Require their implementations and report unavailable operations explicitly;
never fabricate successful graphics/audio/IOP/task initialization. Mark test
adapters as test-only. Preserve original callee return-value semantics and use
a distinct host error channel where needed.

## Verify host semantics and content

Make integer wrapping, sign extension, alignment, endianness and guest-address
resolution explicit. Establish numerical semantics for the actual PS2
instructions: rounding, intermediate operations, denormals and exceptional
values may differ from generic MIPS/IEEE assumptions. Inspect relevant control
register writes and primary implementation evidence. Prefer local explicit
numeric adaptation to changing process-wide FEnv. Do not generalize one RNG
conversion into a model of the whole EE FPU.

Run meaningful known vectors, boundaries and state transitions. Where useful,
compare actual instruction words with native output using a small offline model
or isolated oracle. Record supported instructions, inputs, states and raw return
bits. Reject unsupported instructions instead of inventing semantics. Keep the
oracle outside production; passing it is neither console nor whole-game
validation. Compile/run actual native tests before recording passes. Keep
commands, compiler versions, hashes and reports reproducible.

Read-only asset indexing describes the game input: commands may still create
indexes, reports, caches and build outputs. Respect a request prohibiting those
writes or test execution instead of assuming every verification command is a read.
Index assets/overlays using bounded parsers. Check offsets, lengths
and overflow before reading. Separate container indexing, relocations and
exports/imports from recovered overlay logic. Equal overlay hashes establish
content identity, not recovered source or native execution. Load original data
through a native asset boundary and avoid redistributing it.

Separate upload storage from sampled texture format. Original graphics packets
may write CT32 pixels that are later sampled as indexed textures from the same
VRAM. Verify address permutation, CLUT selection and consumer UVs before using
a generic linear decoder. GPU upload/readback proves transfer correctness;
it does not by itself verify original blending or a rendered game scene.

## Resume and acceptance

Maintain a compact resume reference throughout work: baseline/pin hashes,
modules, exact validation scope, open dependencies, commands and next independent
tasks. Link reports instead of copying large disassembly, decompiler output or
toolchains into the skill. Reconcile mutable reports after resuming; file
existence does not prove success. Preserve rejected assumptions and their
evidence so the next session does not repeat them.

Call the port playable only after its standalone native program loads user
assets, reaches a real scene, renders/plays audio, accepts intended input,
executes original gameplay, and survives agreed transitions/save-load paths.
Define and run an explicit scene/platform acceptance matrix. A parser, DLL,
callback harness or boot slice is progress, not a playable game. Report partial
completion candidly and continue authorized work. This skill grants no external
publishing or tool-installation permission beyond the current user/project scope.

Update the skill during ongoing work when useful. Keep reusable, supported
lessons here; game-specific findings in the case reference; changing status in
the resume reference. Include no game bytes, binaries, ISO/assets, credentials
or copied decompiler output. Revalidate changes with the skill validator.
