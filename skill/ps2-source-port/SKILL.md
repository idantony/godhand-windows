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

For the active God Hand project, start with the newest checkpoint section in
[references/resume.md](references/resume.md). Search its headings or named
artifacts for relevant older results. Use
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

Compile public interfaces with the actual host SDK included first. Legacy
Windows macros such as far/near can erase apparently valid field names; catch
this with an early C/C++ interface check, and keep QA-only driver code guarded
so it also compiles in the ordinary native target.

Trace both caller arguments and callee validation before assigning pointer
types. Two arguments may intentionally alias. A callee can accept a resource
candidate and return NULL when its magic differs; rejecting that candidate as
a host format error changes valid original behavior. A fixture supplying a
recognized format does not establish that the real caller supplies it. Keep
original NULL/false returns distinct from native bounds or implementation errors.

Keep a paused original invocation distinct from a new frame or call. A host
frontier can occur after a list sweep, matrix write or hit insertion; persist
the completed prefix and resume at the failing operation. Retrying from the
start can advance destruction by a frame or duplicate hits. Clear continuation
state only on genuine completion. Rebinding an animation must explicitly begin
a new invocation on the same owner and release its previous sequence/effects;
zeroing the owner is not a valid replacement for the original cleanup.

Treat a typed pointer companion as the current value of an original field,
not an append-only history of assignments. Repeated writes to the same owner
offset must replace that slot while preserving other fields. Exercise scene
reentry: a first room can pass while repeated initialization exhausts a host
cache that the original fixed-field assignment could never exhaust.

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

Run meaningful known vectors, boundaries and state transitions. Turn a
reproducible problem observed twice into a small reusable check, diagnostic or repair
tool, then exercise it on the real case so a third occurrence is caught early.
Keep tool development tied to original-code recovery and actual native execution.
If several original frames or scenes use the same dispatch, batch a bounded
source inventory and run independent native fixtures against the later inputs.
Report the first missing operation with its owner, source row and frame. Keep
empty table slots distinct from invalid offsets. Use the native startup path to
verify the composed result and expose dependencies outside the fixture.
Where useful,
compare actual instruction words with native output using a small offline model
or isolated oracle. Record supported instructions, inputs, states and raw return
bits. Reject unsupported instructions instead of inventing semantics. Keep the
oracle outside production; passing it is neither console nor whole-game
validation. Compile/run actual native tests before recording passes. Keep
commands, compiler versions, hashes and reports reproducible.

As native module counts grow, Windows process argument limits can fail linking
before the compiler starts. Use compiler response files with tested path/token
encoding instead of shortening evidence paths or dropping inputs. Fingerprint
the response writer and record response-file hashes; retain process-launch
errors as failed steps. Exercise an actual long link with spaced Unicode paths.

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

For rendering failures, record the exact consumer, texture table and item,
register state, and referenced vertices. Inspect the matching asset by table
number against an indexed disc entry and log its header and fingerprint. Use
that report to distinguish an unsupported texture profile from a failure
in geometry preparation or sampling.

When preserving a successful controller route for repeated verification,
record raw buttons and axes before the original mapping step. Key playback
to an original gameplay counter, such as completed player updates, after
its original scene or event condition. Wall-clock ticks and frame serials
can drift across decoder and device runs. Reuse the same tape on the
packet-only and device replay paths, and compare their original state
transitions before chasing presentation differences.

For a completed replay check, require every expected gameplay sample and a
terminal report for the intended endpoint. Compare simulation state by gameplay
update and report tick or presentation offsets alongside it. Use partial
overlap to locate the next failure. For reruns, reuse binaries when their
hashes and source pins match and write results to a fresh scratch directory.
Keep the build report in the executable's scratch directory so each build
preserves its own evidence. When verification and Windows builds use separate
scratch directories, pass each directory explicitly to the release check.
Require a passing full verification report, matching source digests from both
reports, and a match to the frozen snapshot receipt. Fail when a selected
report is missing. Explicitly selected extra files must keep any digest
already recorded in a build report.
Record the process exit code even when it produces no replay report.

Use [compare-replay-prefix.py](scripts/compare-replay-prefix.py) with selected
record tags and a shared exclusive tick limit to verify an archived replay
prefix. Its comparison function accepts a live line stream and returns at the
first difference. Keep the replay runner responsible for the process lifetime,
input hashes, and terminal-state verification beyond that prefix.

For a bounded window check that omits intermediate drawing, preserve the
verified startup pacing and compare the original input and state records.
Count omitted frames separately from successful window presents. Resume at a
frame with verified display-history requirements, then read back the presented
target frame. Report missing renderer history as an incomplete capture.

For Windows GPU portability, record the actual adapter LUID and vendor in the
startup log and verify window, UI and model rendering on the selected hardware.
Use the same input and save fixture across adapters. Report gameplay parity
and pixel differences separately. For native sound, distinguish decoded
samples, device buffer completion and audible output in the evidence.

For tests of the ordinary Windows keyboard path, launch the pinned standard
executable with a fresh profile. [win32_keyboard.py](scripts/win32_keyboard.py)
posts focus and key messages only to the hidden window of the owned child.
Use live menu or gameplay readiness when startup timing varies. Preserve the
event timeline and evidence of the resulting original state or rendered frame.
When the application exposes an opt-in typed input trace, use its complete
records to gate menu and gameplay input. Bind the emitted sample to the same
tick's resulting state and keep scheduler serials distinct from present counts.

On Windows, keep command-line arguments Unicode until the native boundary
converts them to the encoding used by the platform APIs. Verify the actual
disc, profile, codec and QA file paths using filenames outside the machine's
ANSI code page.

For a native Windows crash, resolve the failing module and relative address
against the executable's matching PDB before editing. Pass the executable and
the Application Error event's fault offset to
[resolve-windows-crash.py](scripts/resolve-windows-crash.py) for a source line
using local symbols. Audit the failing call
site for host ownership changes introduced by the merge. A shared runtime path
can legitimately lack the pointer used by one actor subtype. Re-run the same
input and verify the original simulation state beyond the failed update.

When reusing a QA observer from an older snapshot, transplant only its
guarded inspection code into the current platform host. Verify the host
outside that block matches production before replaying. Confirm the input
recorded immediately before mapping matches the selected tape through the
last consumed sample. A matching path or input hash alone cannot show that
the probe actually read the tape.

If identical inputs diverge at the first gameplay update, compare original
random state at the same boot, menu and event transitions. The original frame
loop may consume random numbers while a native movie or input prompt waits.
For deterministic QA, reproduce the reference timing through a legitimate
menu input when the trace proves that timing caused the difference. Keep the
random state and gameplay code intact. During a scripted pause, retain changes
to sampled inputs that share one gameplay update index and replay them by
their sample offset within that update.

Use CPU packet replays for rapid source iteration. Before calling a scene
visually verified, capture it from a separate native device run. A bounded run
can prove forward progress when the configured and observed tick limits agree,
the host and renderer are healthy, and the intended frames were presented.
In the God Hand QA launcher, pass `--expect-tick-limit` with `--max-ticks` for
an intentionally bounded observation. The launcher and the evidence recorder
share the completion check for that run, including positive presented frames.

Recover attribute signedness and scale from actual upload commands and math.
Do not assume byte weights divide by255, sum normalization is required, or
normals are renormalized merely because those are familiar graphics conventions.
Trace actual runtime type selectors to their dispatched query and arguments.
Use unequal dimensions to expose swapped radius/height roles; equal-value
fixtures can pass while the original caller binding is wrong.
Keep a recovered geometry leaf separate from palette production, lighting,
projection, clipping and observed native rendering.

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

After a requested milestone upload, review the whole port and its verification
pipeline. Implement and test a bounded improvement to the most consequential
workflow bottleneck the evidence supports. Estimate overall completion against
the agreed acceptance matrix and note the major work still open.
