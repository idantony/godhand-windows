# God Hand case evidence

This is one modified title/release, not a universal PS2 specification. Current
progress/authorization belongs in [resume.md](resume.md).

## Parallel verification and precise stopping

Use immutable authored-source snapshots when workers continue changing shared
headers. Fingerprint every real input, build/test the copy, and stage its exact
verified bytes without overwriting newer working files. A passing copy does not
prove the current working tree. Do not repeatedly invalidate whole long checks
because an unrelated worker is writing another header.

Stop the native developer host immediately on its first unsupported task. An
old host waited for unrelated live fibers to finish before surfacing failure;
this accidentally made asset-completion/tick-count tests pass. Assert the true
frontier and canonical state; verify real asset completion separately in the
integrated frontend run. Never retain a synthetic delay just to satisfy a test.

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

## Later integration corrections

- ID path lengths are floating-point: raw LWC1/ADD.S/SWC1 contradict a decompiler
  integer cumulative-length guess. Native EXACT policy rejects unsupported
  non-square roots; the explicit REFERENCE_NEAREST profile uses pinned PCSX2
  reference semantics. Neither profile is a hardware capture. The HUD actually
  needs the reference policy. See analysis/id-path-semantics.json.
- Preserve global identity across recovered subsystems. GamePrepare's span at
  005E8658, stage WorkArea at005F3970, VBS at005E80E0, child storage at00566E50
  and Cockpit at00464080 are distinct. Do not enlarge/merge similarly named
  storage to make an offset fit. Camera reset borrows GameInit's actual camera.
- 00307388 is screen-fade drawing, not pad input. Actual digital input follows
  0031A808/0031B560/0031AFE0 and two original ELF mapping tables. US table1 maps
  Cross/Circle to confirm and Triangle to cancel. Write original raw32/logical64
  fields explicitly; host long/pointer widths are unsuitable.
- Real native MC rendering uses original job order and dark palette. Styling
  text by taste or reordering the window changes original behavior. Font nearest
  sampling and T32 sprite4-bit bilinear need different restricted GPU pipelines.
  Pixel agreement applies to the explicit supplied GS state, not every GS mode.
- The true origin SFX entry is002CB8A0;002CB968 is a different overload. Program
  to sample mapping follows the actual descriptor chain, not a guessed offset.
  Device submission, actual completion and audible QA are separate observations.
- Original Movie0 requires video EOF and real audio-buffer completion. A timer
  alone cannot report finished. Native QA may mute its own PCM/output gain while
  still running the actual decoder, device queue and completion tracking.
- In this FST, configured directory token Release can alias the first actual
  child directory (debugoptimize). This comes from001FF858/001FFA68, not a general
  rule to strip missing directories. Exact gh_disc_find does not implement it;
  a typed overlay loader must validate the actual canonical association.
- Upstream recon/rel_inventory.py's relocation addend unpack is unreliable for
  nonzero addends. R020 exposes the issue: inspect the real12-byte record and
  original loader; the addend is at+8, not the script's overlapping+6 read.
  Earlier r207 zero-addend records did not expose this. Inventory is evidence
  about names/structure, not proof that guest relocations or code can execute.

Independent task managers share one stable root fiber. A Scenario traversal
from a running Main task needs explicit nested entry and caller restoration;
it must not replace Scenario16 records with Main4 records. Destroy Scenario and
its runs before root backend/runtime/assets. See test_nested_tasks.c and the
GameServices ownership tests. A successful module test is not a full boot.

Outer-loop0030AC40 writes FPS at Work+568 with SB, not SW. The next byte+569 is
the independent frame-mode selector. Poisoned-neighbor tests caught a draft
word-store mistake; actual instructions resolved it. Work+564 controls fade
duration shifting. Movie update precedes task traversal; Fade307388 follows
tasks, before DVD and audio.31A338 builds OT chains, not pad input. Frame MT
advance consumes two words before tasks, sharing GameInit's MT owner. Actual
31CEB8 seeds MT withD8A9523D and LCG66B; using MT's uninitialized fallback5489
as the game's observed boot seed was incomplete. The developer host still
does not claim all intervening boot/constructor behavior or hardware cadence.

Work+5F4 is the original GetPlayer identity (2DB6B0/31C4E0), not a clock. The
first-room Cockpit reset precedes player creation and sees NULL. Later nonzero
tokens need the canonical typed actor owner; never treat them as time or cast
32-bit guest data into a host pointer. Some early provisional notes/tests called
this a clock; later primary call/store evidence rejected that interpretation.

Sound-scene ratios60/74/88 and related fields really remain BSS zero through
2C9F58. Its2CA220 call has no cSnd argument and performs ADX platform setup,
not initialization of those arrays. Bind one early native owner at startup and
run real2CAF28 updates before2D0938 BGM and later SE flush; services must borrow
that owner rather than construct a fresh room-local copy.


Two explicit native memory profiles are now required for observed original
uninitialized inputs. They are documented adaptations, not PS2 captures:

- ID paths 002AC6E0/002AC878 read an unwritten cumulative-distance stack slot.
  Strict APIs retain the frontier. The opt-in animation profile supplies a
  zero distance origin; a supplied nonzero origin must actually affect output.
- R020 title player constructor 00107E40 does not write vitality at +54A.
  Strict allocation leaves this unknown and checkpoint stops at 001F9BEC.
  ZERO_REFERENCE supplies allocation bytes before the unchanged constructor;
  original checkpoint clamp turns HP0 into HP1. PROVIDED_BYTES preserves a
  caller's concrete memory image. Never manufacture normal gameplay HP100.

The encrypted ADX decoder derives the actual key through the user's ELF
callsite/string/prime table. Full title PCM matches independent vgmstream;
the native waveOut provider owns bounded buffers, actual completion, pause,
seek, stop and looping. This is a native codec/platform boundary, not an
external decoder process or a complete SPU2 equivalence claim. BGM Set prepares
the stream; it must wait for the original Start request before playing.

Immediate ID consumer submission order is not yet proof of final GS display
order. Final OT insertion/traversal can change composition. Keep the palette
and submission evidence, audit 0031A338/0014FE78/0014FF60/0015FBA8, and do not
reorder or brighten the MC window based on appearance alone.


Resolve every global flag address against the actual Work base007474A0 before
naming a field:00747A24 is +584,00747A78 is +5D8,00747A80 is +5E0. Upstream's
phrase "system flags" is not a sufficient offset. The camera139C88/MoveObj
D_00747A78 gate is distinct from the startup/Main flags584 word. R020's real
5D8 OR F00C6000 is relevant to that gate; do not bind an invented tracked-player
pose merely to get past a mistaken flag offset. Recheck raw loads before use.

The four canonical754C30/38/50/58 object-pointer registries are already the
four vectors in game_room_pools, created by actual2A5D80→3127B0 from RHT.
They are distinct from global80/room32 asset holders. Use gh_room_registry_view
on the existing initialized pools; do not create fresh empty list companions.

The decompiler can misgroup computed switch targets even when the instruction
bytes are verified. Enemy20A behavior2064FC is a concrete case: actual table
42E8C0[index8] selects20654C, whose branch/delay slot writes17B9=0; generatedC
had grouped20A with a value3 case. Verify the local table base/index/value and
target raw control flow before importing a selector. The bounded offline trace
records opaque callee assumptions and is not whole-method native verification.

NearCheck2A6FF8 precedes dynamic-box12E840. Constructor12D970 defines zero
bounds/corners; original130700 tests AABB and face masks/exclusions BEFORE
consuming corner geometry. Requiring hostworld_ready before those earlyouts
caused a false12DCB8 frontier. Surviving faces must still require actual geometry;
neither force an early update nor default an unknown query to a miss.

MoveObj30F090 caches its iteration gate/end, calls the original descriptor,
then reloads the current registry slot at30F16C before clearing2F0 lowtwo bits.
Native continuations need a separate completed-call/post-call phase: an error
resolving that reloaded slot must not execute the completed Move again.

Large-function evidence must record the raw instruction limit and explicit
truncation.104010 is2175 instructions: a2048 capture is incomplete even when
Ghidra produced C. Normalize requested addresses before expanding callees;
upper/lowercase duplicates can otherwise obscure the exact selected body.
Keep earlier incomplete reports and label the authoritative replacement.

Positive motion blend ordering is observable:157C18 decrements438, samples
158328 with the PREVIOUS27C, then stores the next factor for149EB8 matrix
blend. OldSEQ release, old world-relative pose capture and new motion binding
are distinct phases. A late native error must resume the same invocation,
without repeating released effects, RNG, decrement or completed bone sampling.

HitArea's native last-hit actor companion replaces the current original34
identity. Validate its lifetime/generation; keep rawguest tokens separate from
host pointers. Source and target views must resolve the same registered owners.
The real actor may aliasF0 to a part: reading rawF0 bytes as a position can give
a plausible wrong ray. A genuine original earlyout may avoid reading unrelated
player pause globals; require them only on a branch that actually consumes them.
