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

The current Windows QA build repeats the controlled R105 test. Direct3D
presents Game Over at scene 4230 and the reloaded R105 at scene 4322 with
100 HP and zero renderer errors. The standard Windows executable also loads
an R102 save through the original menus, presents 539 scene frames, and
registers movement from ordinary keyboard input. The test save files retain
their original bytes.

Another actual-disc CPU replay restores the R100 checkpoint from Game Over and
registers movement and a Square attack after the reload. An original controller
recording reaches the AT10 area with Gene alive in a native CPU replay.

A separate integration test followed the continuous West route from New Game
through all 28 EV02 cuts in R003 and entered R105 at tick 20,053. The native CPU
run matched 16,007 archived startup and gameplay records. It stopped at an
unfinished R105 enemy attack branch. A continuous Direct3D replay on the same
route reaches that stop in R105 and presents 20,122 scene frames with zero
graphics errors.

A later private replay selects the original R105 Game Over Retry and
returns to gameplay at 100 HP. A Direct3D diagnostic restores captured
display history and presents scene 20,140 with the original HUD and
enemy-marker ordering.

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
enemies and HUD.

A newer controlled Windows run completes the same transition and continues
through the first 36 R102 presentations. After the original Continue menu,
24 stick inputs move Gene 3.08 scene units with 100 HP. The run verifies 16
consecutive captures with matching pre-UI frames and zero renderer errors.

The original save menu at the end of Event 04 writes a valid R102 checkpoint.
A fresh launch loads that checkpoint through the original title and save-map
flow. Direct3D presents 36 consecutive R102 frames while a controlled stick
input moves Gene. Two independent title-load runs also present the first
verified R102 melee hit and its follow-through. Square lowers the targeted
enemy from 74 to 71 HP. Each run captures the same 32 consecutive Direct3D
frames with matching world and HUD-composited images. In a separate CPU
replay from New Game, the same enemy's counterattack lowers Gene from 100
to 91 HP.

A controlled native replay from the R105 exit writes a fresh R102 slot
through the original Event 04 save menu. A separate launch of the standard
Windows executable loads a byte-identical copy through the original title
menu, presents 539 frames, and moves Gene 2.83 scene units with keyboard
input. A Direct3D combat replay from the same new save reproduces the
Square hit. The save data and both icons retain their original bytes.

A native CPU replay loads the same fresh save and follows recorded controller
input through the R102 exit and the original Continue menu. It reaches R103
with scene-frame serial 11,632 and no unfinished call at the planned tick
limit. A separate Windows QA run follows the same input and switches to
Direct3D before the end of Event 1A. It presents 26 frames, including eight
R103 frames after the event, with zero renderer or window errors. The copied
save is unchanged. The resulting images differ from the earlier build.

A later R102 title-load replay presents 88 Direct3D frames covering an enemy
attack, Gene's hit reaction at 45 to 36 HP, and the enemy's recovery. A CPU
test starts with the mid-room gate's defeat conditions set in the card and
verifies the gate sequence with five reinforcements over 120 room frames.
The integrated New Game CPU replay reaches 1,113 R102 room frames at its
planned limit.

A later native replay starts from an isolated R102 card with the gate's defeat
conditions set. Stick input follows the original corridor and Circle activates
the original exit. The replay chooses Continue in the original save map, enters
R103 through its original door spawn, restores the carried paper, and completes
all 11 EV1A shots with Gene at 100 HP. It runs more than 840 R103 CPU frames
before reaching the original arena setup. A Windows replay uses native CPU
title setup, then Direct3D presents all 776 R102 frames in the route and 12
consecutive R103 frames after the transition, with zero renderer errors.
A longer replay switches to Direct3D shortly before the R102 exit and
presents 1,122 consecutive R103 frames. It presents all 11 original EV1A
shots, returns to the arena with Gene still carrying the paper from R102,
and reaches the first enemy hit, which lowers Gene from 100 to 91 HP.
The replay reports zero graphics errors. A separate input replay through
the same exit presses Square and lowers the first enemy from 74 to 71 HP.

Another saved R102 route plays all 12 original EV1B shots and resumes
gameplay. After a lethal enemy hit, the replay selects Retry in the original
Game Over menu. Gene reloads the checkpoint at 100 HP, and Direct3D presents
64 consecutive frames after the reload.

Native CPU replays also verify R100's original Game Over and Retry flow and
R101's box throw and 200 G pickup. R100 reloads with 100 HP and continues
for 120 gameplay frames. A longer R101 replay runs for 4,494 native CPU
frames and verifies the box and item releases, the original DIE marker
and enemy audio. R101's later combat presentation remains in development.

A later R100 controller replay reaches Game Over after a West encounter.
The original Cross confirmation increments the Continue count. The room
retires the stopped Heat effects and the Devil before reloading. The native
CPU replay restores Gene to 100 HP and records three updated player frames.
The Windows QA replay switches to Direct3D before the menu and presents
224 frames, including Game Over and the reloaded R100 scene at 100 HP.
An extended replay presents 350 frames through renewed combat. After control
returns, 24 left-stick samples move Gene 1.05 scene units at 100 HP. Later
combat takes Gene to 82 HP before the bounded replay ends in R100.
On a separate native CPU replay, Gene approaches the opening enemy after
Retry and presses Square. The original hit takes the enemy from 74 to 71 HP
while Gene stays at 100 HP. The replay reaches the enemy's next response.

The new-game east route now breaks the original door, continues into the
armed encounter, and reaches the original Game Over menu after a lethal
hit. Confirming Retry restores Gene to 100 HP in R100. A separate west-route
replay reaches the original reinforcement area and lands two Triangle hits
during its Devil encounter. A controller replay from a zero-defeat R102
checkpoint records all six qualifying defeats. Gene collects the original
attack-boost drop after the fifth defeat and lands the sixth by escaping a
grab and countering. With 9 HP, he continues through the original exit and
confirms Continue on the save map. The same replay enters R103 and produces
12 consecutive frames with native world and actor packets. The loaded save
has the same SHA-256 before and after the run. An extended replay on the
same source completes all 11 EV1A shots and verifies analog movement after
the first arena wave starts. Direct3D presents 86 R102 and 10 R103 scene
frames around the exit, plus eight consecutive arena frames after EV1A and
its fade finish. Both Direct3D runs report zero renderer errors.
The continuous replay also reaches the original Game Over menu in R103.
After confirming Retry, Gene returns to the arena at 100 HP and defeats both
first-wave enemies. The original encounter ledger records both defeats. Gene
has 56 HP after the second defeat. Direct3D presents 118 scene frames through
the second defeat with zero renderer errors.
A longer replay from the same save defeats all six enemies across the first
three waves. The original encounter ledger records all six defeats. Gene
enters the original twin-boss introduction with 24 HP. Both bosses have
621 HP, and left-stick input moves Gene after the camera releases control.
Direct3D presents 79 scene frames around the boss entrance with zero
renderer errors.
A native CPU replay loads an isolated R103 test save and uses original pad
inputs to defeat all six enemies in the first three arena waves. Gene takes
the original healing drop and enters the twin boss wave with 58 HP. The replay
submits render commands for the boss models and continues into their original
attack selection. A muted Direct3D replay presents 6,908 scene frames through
the original boss entrance with zero renderer errors. A separate controlled
fight starts with the first three
waves cleared and verifies three hits from the first boss's original combo.
Gene's HP drops from 100 to 87. On the replay that clears all six enemies,
the original paired throw lowers Gene's HP from 58 to 40. He gets back up
and regains control. On one route, Square lowers the second boss's HP from
597 to 594. On another route, Triangle lands twice, lowering the second
boss's HP from 597 to 573.
The boss then evades and lands the original roundhouse. A muted Direct3D
replay presents the original paired throw.
A separate controller replay reaches Game Over after a lethal paired throw.
Confirming Retry restores Gene to 100 HP and returns him to the R103 battle.
The coordinated flank attack also runs through contact and withdrawal.
It lowers Gene's HP from 31 to 17 and then to 12 on the fall. A separate
right-stick dodge avoids that attack and leaves Gene at 31 HP for the next
exchange.
Two native R103 title-load replays escape the first boss's sustained grab
with real button presses and restore player control. Leaving the pad neutral
during the grab plays the original lethal motion. Pressing Cross in the
original Game Over menu reloads R103 at 100 HP and resumes scene updates.
The same input tape presents 106 Direct3D scene frames covering the menu and
the first visible scene after Retry. In a separate fight, an amplified Triangle
hit lowers the second boss from 549 to 525 HP, followed by another hit to 513.
A right-stick dodge during the original attack cancel window avoids the first
boss's next grab attempt.
Full boss clearance remains in development. Earlier Direct3D replays present the escape counter and the movement that
follows it after entering R103 through the original door from R102.

A separate test starts with all enemies marked defeated and places Gene at
the original interaction areas. Circle opens the arena's post-battle dialogue,
Cross closes it, and Circle at the original return door loads R102. The native
CPU replay completes 120 R102 frames. Both original English post-battle
messages pass drawing and dismissal checks.

### Verified progress

The native Windows replay confirms the R021 results with the original Cross
input, awards 50,100 G in the verified case and renders EV07 through its
final shot. A scheduler replay completes all 22 authored shots, releases
the scene and returns to R021.

The Stage 2 map shows the route and the original Save, Continue, Enter Shop
and Enter Casino options. The Continue path enters R003 and prefetches the
original EV08 audio. A separate replay uses the original Start shortcut
during EV07 and writes a new card slot through the original Save menu.
A fresh process reads that slot from the title menu and enters R003. The
record contains 50,100 G and the Stage 1 unlock flags. Overwrite confirmation
replaces the selected record and preserves the other slot and menu icons.
Declining leaves both slots unchanged.

The native Continue replay completes EV08 and enters the original R200
corridor. Once the opening camera releases control, the left stick moves
Gene in the rendered scene. The replay continues through the first catapult
launch, flight and impact. Separate checks verify the original impact
effects and collision-area cleanup.

Two Windows replays load an original-format R200 diagnostic save and verify
combat in both directions. A Triangle input hits two enemies twice each,
bringing both from 96 to 72 HP. On a separate route, EM1A's attack takes
Gene from 100 to 92 HP and triggers the original hurt animation.
A longer Windows replay walks past the opening attackers and activates
the original road trigger. It records the room flag change and starts the
next two enemies while Gene stays at 100 HP. A follow-up replay presents
1,446 Direct3D scene frames through the encounter. Original enemy hits take
Gene from 89 to 78, 67 and then 56 HP, with no graphics errors recorded.

The progression replay uses the earlier EV07 Start shortcut and sets the
boss to 60 HP immediately before the original 80-point Shockwave. A separate
full-health combat replay raises Heat with an L2 taunt and lands two hits
after evading the counterattack. The full-health victory and the rest of
Stage 2 remain in development.

The Windows port now renders combat in both the R104 boss arena and the R101
side arena. In R104, a recorded Triangle input hits the boss after the original
intro. A second replay enters through the original keycard gate and verifies a
boss projectile's launch, flight, hit and removal. Gene's HP drops from 100
to 80 on contact. A right-stick evade on the same approach avoids the hit.
In R101, a native CPU replay enters the side arena from an unchanged
zero-card save, clears the four original waves and picks up the keycard
after defeating the pair of bosses. The carried count changes from zero to
one. Windows presentation of the later waves remains in development.

The original R104 Retry menu now accepts a recorded Cross input after Gene's
defeat. The native replay retires the old room, restores the checkpoint and
resumes the fight with Gene at 100 HP and the boss at 1,000 HP. A separate
Direct3D replay presents the second hit on the boss as its HP reaches 991.
In R100's west area, a replay earns and activates Heat and records ten of the
twelve defeats required by the original gate.

After R104 Retry, recorded controls exercise both outcomes of the boss's hold.
Alternating Cross and Triangle frees Gene at 100 HP. He can then walk and
start another Triangle attack. Waiting in the hold triggers the original
impact and recovery, leaving Gene at 77 HP. Direct3D also presents the hold
and its distortion effect. A separate test save initialized
with the original New Game equipment verifies two 12-point contacts from one
Triangle press, taking the boss from 1,070 to 1,046 HP.

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
- The current regression build passes 390 native checks.
  This run includes muted hardware audio and movie decoding. Earlier runs also
  exercised the two optional independent ADX sample comparisons.
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
R103 runs the original reinforcements and opens the twin-boss fight. Recorded
controller input verifies Heat activation, damage to both bosses, and return to
normal movement. A Direct3D 11 replay shows the aura appear and expire. A
controlled test lowers the bosses' HP before landing real attacks and verifies
both defeat records, the arena-clear flag, and the original item drops.
In that controlled fight, each boss is set to 12 HP before the finishing
attack. Recorded controller input then walks Gene to the original return door
and presses Cross. Direct3D 11 presents 587 scene frames across the test. Once
the return fade clears, Gene is visible in the R102 corridor at 90 HP with the
original HUD and zero renderer, GPU or window errors. A separate full-HP replay
verifies the original flank charge and the boss's rear Triangle recoil and footstep.

The combined build verifies eight registered defeats on the original R100
west route, with Gene at 1 HP. In R103, a separate controller replay starts
with both bosses at 597 HP. Back + Circle and its collateral hit lower their
HP to 572 and 587. The first Roulette Shockwave hits both bosses for 80.
A second Shockwave and its recoil collision leave them at 412 and 487 HP,
with Gene alive at tick 9,000. Direct3D presents 462 frames across the first
two attacks with zero renderer errors. The complete full-HP fight remains in
development.

The next fixed controller replay activates Heat after both Shockwaves.
Heat expiry hits both bosses, and a later Triangle hits each twice. At
tick 9,650 the bosses have 450 and 383 HP, with Gene at 7 HP. A separate
fixed West route records nine original defeats by tick 10,208, with Gene at
1 HP. The active West gate calls for twelve defeats.

Direct3D 11 now presents the original R104 EV05 intro from an original-format
R104 test save prepared before launch. After the original fade clears, the
first shot shows Gene facing the boss. The fixed run renders through camera
cut 9 with zero renderer, GPU, effect or window errors. A longer Direct3D
replay verifies the original English captions. The native CPU replay finishes
EV05 and enters R104's original boss initialization.

A separate Windows replay starts from an original-format R101 test save
prepared with three keycards and a test landing position. Recorded controller
input opens the original door, selects Continue on the save map, and enters
R104. Direct3D presents 1,170 scene frames across the run with zero graphics
errors, including the R104 boss intro. In a longer native replay from the same
R101 save, the intro ends and left-stick input moves Gene using the original
walk state. A separate Direct3D replay from the R104 test save shows Gene
walking in the boss arena after the fade clears. The R103 twin-boss replay
also verifies two forward evades that preserve health and award Heat and
level points. Collecting all three cards during play and completing the boss
fights remain in development.

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

The window, model and UI renderers select one hardware adapter for the process.
They prefer a D3D11 feature level 11.0 NVIDIA adapter, with other capable
hardware available as a fallback. Set `GODHAND_D3D11_ADAPTER_INDEX` to a DXGI
adapter index to select it explicitly. The startup log records the selected
vendor, device and LUID.

The native Windows command accepts Unicode paths for the ISO, profile and
FFmpeg.

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
Use WASD for movement and the arrow keys for the D-pad. Z maps to Cross,
X to Circle, C to Square, Escape to Triangle and Enter to Start. The original
ELF tables determine logical confirm and cancel.
Use `--trace-native-input` to write the accepted input and resulting menu,
room and player state to standard error. It also records the cumulative
number of scene presents.
The R100 QA pad reader accepts both recorded CSV formats, including the
version with analog triggers.
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
To reproduce the saved route, create a checkpoint with the CPU QA executable
using its usual arguments, a fresh empty card directory and `--r102-save`.
Pass a copy of that profile to `startup-qa` with `--qa-load --qa-r102-walk`
and `--qa-capture FILE --qa-capture-count 36`. The game reads the checkpoint
through its original title menu.

On Windows, the card backend stages each save file in its slot directory.
A successful close publishes the complete file. The isolated regression
`python -B tools/build-native.py --focus mc-file-save` runs without an ISO.

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
恢复到 100 HP。单独的无画面原盘回放在这次重载后又完成 60 帧。

当前 Windows 验证构建复跑了受控的 R105 流程。Direct3D 在第 4230 帧呈现 Game Over，
并于第 4322 帧呈现重载后的 R105。Gene 恢复到 100 HP，渲染错误为零。标准 Windows
程序还通过原版菜单加载了 R102 存档，呈现 539 帧，并响应普通键盘移动。测试存档文件
保持原有字节。

另一轮无画面原盘回放恢复了 R100 检查点，重载后接收移动和方块攻击。一轮原盘无画面回放
通过录制的原版手柄输入走到 R100 的 AT10 区域，Gene 仍然存活。

一轮独立集成测试从新游戏沿西区连续推进，在 R003 播完 EV02 全部 28 个镜头，于第 20,053 个
tick 进入 R105。原生 CPU 回放与归档中的 16,007 条启动及游戏状态记录一致，随后停在 R105
敌人一个尚未覆盖的攻击分支。相同路线的连续 Direct3D 回放也到达 R105 的同一处，呈现
20,122 个场景帧，图形错误为零。

后续私有回放确认 R105 原版 Game Over 的 Retry，重进关卡后 Gene 恢复到 100 HP。
Direct3D 验证从已呈现的画面恢复显示历史，并按原版 HUD 和敌人标记顺序呈现第 20,140 帧。

另一轮 Windows 回放从标题读取 R101 的原版格式测试档，经原版章节地图确认，在第 461 帧呈现
R101。运行到预定的 800 tick 上限，完成 187 次玩家更新，渲染无报错。独立的无画面回放也
验证了摇杆移动和方块攻击。更长的 Windows 回放使用原版手柄映射打开 R101 的门，并走进门后的走廊。

一轮受控的 Windows 回放从 R105 已开门的存档状态开始，并把 Gene 放在原版出口。原生音频
播放开门声，原版房间切换载入 R002。Direct3D 连续呈现了 R002 的 3,390 帧，完整播放 EV04 的
24 个镜头并显示原版保存界面。回放用原版按键选择继续。后续回放连续呈现了 R102 的前 16 帧，
显示原版场景、Gene、敌人与 HUD。

新一轮受控 Windows 回放完成了同一转场，并继续呈现 R102 的前 36 帧。通过原版菜单选择
继续后，24 次摇杆输入使 Gene 移动约 3.08 个场景单位，生命值保持 100。回放核验了
连续 16 张截图及其对应的界面合成前画面，渲染错误为零。

EV04 结尾的原版保存菜单已写出有效的 R102 存档。重新启动后，通过原版标题菜单和存档地图
读入该存档。测试用摇杆让 Gene 移动，Direct3D 连续呈现 R102 的 36 帧。两次独立读档
还验证了 R102 首次近战命中，方块攻击将目标敌人的生命值从 74 降到 71，并播放原版受击动作。
两次回放各捕获 32 个连续的 Direct3D 帧，对应的场景和 HUD 合成画面逐帧一致。
另一轮从新游戏开始的 CPU 回放验证了同一敌人的反击，Gene 的生命值从 100 降到 91。

一次从 R105 出口开始的受控原生回放，在 EV04 的原版保存菜单写入了新的 R102 存档。
标准 Windows 程序重新启动后，通过原版标题菜单读取该存档的逐字节副本，呈现了 539 帧，
并响应普通键盘输入让 Gene 移动 2.83 个场景单位。另一轮 Direct3D 回放从同一个新档
复现了方块命中。存档数据和两个图标保持原有字节。

原生 CPU 回放读取同一个新档，用录制的手柄输入通过 R102 出口，在原版菜单选择「继续」。
回放到达 R103，达到预定上限时场景帧编号为 11,632，未遇到未实现调用。
另一轮 Windows 验证使用同样的输入，在 EV1A 结束前切换到 Direct3D，实际呈现了 26 帧，
其中 8 帧位于过场结束后的 R103。渲染与窗口错误均为零，存档副本保持原样。
采集画面与较早构建存在像素差异。

后续 R102 读档回放呈现了 88 个 Direct3D 帧，覆盖敌人攻击、Gene 从 45 HP 降到 36 HP 的
受击反应，以及敌人收招。CPU 测试用预置击败条件的存档验证了中段门和五个增援，运行了 120 个
房间帧。合并版本的新游戏 CPU 回放在 1,113 个 R102 房间帧达到预定上限。

后续原生回放从预置击败条件的隔离 R102 存档开始。摇杆沿原版走廊移动，圆圈打开原版出口，
在原版保存地图选择「继续」后从原版门口进入 R103。回放恢复了随身纸片，并以 100 HP 播完
EV1A 的全部 11 个镜头。进入原版竞技场初始化前已运行超过 840 个 R103 CPU 帧。
Windows 回放用原生 CPU 完成标题流程，随后 Direct3D 呈现路线中的全部 776 个 R102 帧，
转场后又连续呈现 12 个 R103 帧，渲染器报错为零。另一次回放在 R102 出口前切换到
Direct3D，进入 R103 后连续呈现 1,122 帧。回放播完原版 EV1A 的全部 11 个镜头，进入竞技场时
Gene 仍带着从 R102 拿到的纸片，随后第一下敌方命中将他的生命值从 100 降到 91。
回放未报告图形错误。另一轮回放经过同一出口，按方块命中第一波敌人，将其生命值从 74 降到 71。

另一条 R102 读档路线播完原版 EV1B 的全部 12 个镜头并回到可操作状态。Gene 被敌人击倒后，
回放在原版 Game Over 菜单选择重试。他以 100 HP 读回检查点，随后 Direct3D 连续呈现 64 帧。

原生 CPU 回放验证了 R100 的原版 Game Over 和重试，以及 R101 的投箱和拾取 200 金币。
R100 重载后以 100 点生命值开始并继续运行 120 帧。较长的 R101 回放运行了 4,494 个原生 CPU 帧，
验证了箱子与掉落物释放、原版 DIE 标记和敌人音效。R101 后续战斗的画面仍待验收。

另一轮手柄回放在 R100 西侧遭遇战后进入原版 Game Over。按叉键确认后，Continue 计数加一。
重载前释放已停止的热力特效并退役 Devil。原生 CPU 回放中，Gene 恢复到 100 HP，
重载后又完成了三次玩家更新。
Windows 验证在菜单出现前切换到 Direct3D，实际呈现 224 帧，捕获了 Game Over
菜单和以 100 HP 重载后的 R100 场景。
延长回放实际呈现了 350 帧，覆盖重载后的交战。恢复控制后，24 次左摇杆输入使 Gene 在
100 HP 时移动 1.05 个场景单位。随后交战让 Gene 降到 82 HP，回放在 R100 达到预定结束点。
另一条原生 CPU 回放在重试后用左摇杆接近首名敌人，再按一次方块。原版攻击命中后，
敌人从 74 降到 71 HP，Gene 保持 100 HP。回放推进到敌人的后续回应。

从新游戏开始的东侧回放打破原版木门，在后续持械战斗中被击倒，并在原版 Game Over 界面
确认重试，以 100 HP 回到 R100。西侧回放进入原版增援区域，在恶魔遭遇期间打出两次三角攻击。
R102 从零击败存档开始，经手柄回放完成开门所需的全部六次击败。Gene 拾取第五次击败后掉落的
原版攻击强化，并用挣脱反击完成第六次击败。他带着 9 HP 走到原版出口，在保存地图确认继续。
同一条回放进入 R103，连续生成 12 帧原生场景与角色绘制数据。载入的存档在回放前后哈希一致。
延长回放播完 EV1A 的全部 11 个镜头，验证首波敌人启动后的摇杆移动。Direct3D 在转场附近
呈现了 86 个 R102 场景帧和 10 个 R103 场景帧，在 EV1A 及淡出结束后又连续呈现了 8 个竞技场帧。
两项 Direct3D 回放的渲染器报错均为零。
同一条连续路线还在 R103 进入原版 Game Over 菜单，确认重试后以 100 HP 回到竞技场，随后
击败首波两名敌人。原版遭遇记录登记了两次击败，第二次击败后 Gene 还剩 56 HP。
Direct3D 呈现了直到第二次击败的 118 个场景帧，渲染器报错为零。
从同一份存档延长回放后，打完前三波共六名敌人，原版遭遇记录登记了全部六次击败。
Gene 带着 24 HP 进入原版双头目入场，两名头目各有 621 HP。过场交还控制后，可用左摇杆移动 Gene。
Direct3D 呈现了双头目入场前后的 79 帧，渲染器报错为零。
原生 CPU 回放从独立的 R103 测试存档开始，用原版手柄输入击倒前三波共六名敌人。
Gene 拿到原版恢复道具，以 58 HP 进入双头目战。回放提交两名头目的模型绘制指令，并运行到
原版选招。静音 Direct3D 回放呈现了 6,908 个场景帧，覆盖原版头目入场，渲染器报错为零。
另一项受控测试预置前三波已清场，验证首名头目连击三次命中，Gene 从 100 HP
降到 87 HP。清完六名敌人的回放继续进入双头目战，原版合投让 Gene 从 58 HP 降到 40。
Gene 起身恢复操作后，方块路线将第二名头目的生命值从 597 降到 594。
三角路线连续命中两次，将第二名头目的生命值从 597 降到 573，随后验证了头目的闪避和回旋踢。
静音 Direct3D 回放呈现了合投过程。另一条手柄回放在 Gene 被合投击倒后进入原版 Game Over 菜单，确认重试后
以 100 HP 回到 R103 继续战斗。原版夹击已运行至侧翼命中和收招，Gene 的 HP 从 31 降到 17，
倒地时再降到 12。另一条路线用右摇杆闪避这次攻击，以 31 HP 进入下一次交锋。
两条从标题读档进入 R103 的原生回放通过连续按键挣脱首名头目的持续抓投，恢复操作。
抓投期间保持手柄无输入会播放原版致死动作。在原版 Game Over 菜单按叉键重试，
Gene 以 100 HP 回到 R103，场景继续运行。
同一份手柄回放用 Direct3D 呈现了 106 帧，覆盖失败菜单和重试后的首个可见场景。
另一条路线中，强化三角命中将第二名头目的 HP 从 549 降到 525，随后再次命中降到 513。
在原版攻击允许取消的时机使用右摇杆，成功躲过首名头目的下一次抓取。
完整头目战仍在开发中。较早的 Direct3D 回放从 R102 经原版门进入 R103，
已呈现挣脱反击和随后的摇杆移动。

另一次测试预置全场已清，并将 Gene 放到原版交互区域。用圆圈打开战后对白，叉键关闭，
再在原版返回门按圆圈加载 R102。原生 CPU 回放进入 R102 后连续运行 120 帧。
两条原版英文战后对白均通过绘制和关闭测试。

### 已验证的进展

原生 Windows 回放用原版叉键确认 R021 结算，测试路线领取了 50,100 G，并渲染 EV07
至最后一镜。调度回放跑完原版的全部 22 个镜头，释放过场对象后返回 R021。

第二关地图显示路线和原版的保存、继续、进入商店、进入赌场四个选项。选择继续会进入
R003，并预读原版 EV08 音频。另一条回放用原版 Start 跳过 EV07，再通过原版保存菜单
写入新槽位。新进程从标题菜单读入该存档并进入 R003，存档中的 50,100 G 和首关解锁
标记均已核对。确认覆盖会替换当前槽位并保留另一槽位及菜单图标。取消覆盖后，两份
存档均保持原样。

原生流程回放从继续选项进入 EV08，过场播完后切换到第二关的原版 R200 走廊。
开场镜头交还控制后，左摇杆能让画面中的主角走动。回放跑过了第一发投石机的发射、
飞行和落地。单独的检查也验证了原版落地特效和命中区域清理。

两条 Windows 回放加载原版格式的 R200 诊断存档，验证双方都能命中。按一次三角键，
两名敌人各被命中两次，血量都从 96 HP 降到 72 HP。另一条路线中，EM1A 的攻击使
Gene 从 100 HP 降到 92 HP，并触发了原版受击动作。
一条更长的 Windows 回放走过开场交战区，触发了原版道路事件。房间进度标记随之写入，
下一批两名敌人入场，Gene 保持 100 HP。后续回放用 Direct3D 呈现了这场战斗的 1,446 帧，
原版敌人的几次攻击让 Gene 的血量从 89 依次降到 78、67 和 56 HP。全程记录的图形错误为零。

这条流程回放此前用了原版 Start 快捷键跳过 EV07，并在原版 80 点冲击波命中前将
头目血量设为 60 HP。另一条满血战斗回放通过 L2 挑衅积攒斗气，闪避反击后命中两次。
满血获胜和第二关后续流程仍在开发。

Windows 端已能呈现 R104 头目战和 R101 支线竞技场的原版战斗。R104 播完原版过场后，
回放中的三角键攻击命中头目。另一条回放经原版钥匙卡门进入 R104，验证了头目的普通弹
从发射、飞行到命中和清理的流程。Gene 中弹后从 100 HP 降到 80 HP，同一路线加一次
右摇杆侧闪即可避开。R101 的原生 CPU 回放从未编辑的零钥匙卡存档进入支线竞技场，
打完原版四波、击败最后两名头目并拾起钥匙卡，持有数量从零变为一。后续几波的 Windows
画面呈现仍在开发中。

R104 的原版重试菜单已经能在 Gene 战败后接收回放中的叉键。原生流程清理旧房间、
恢复检查点并继续战斗，Gene 回到 100 HP，头目回到 1,000 HP。另一条 Direct3D
回放呈现了第二次命中，头目血量降到 991。R100 西区的回放通过原版输入积攒并激活
热血，已记入开门所需的十二次击倒中的十次。

R104 重试后，原生手柄回放验证了头目抱摔的两种结果。交替按叉键和三角键，
Gene 能以 100 HP 挣脱，随后可以移动并再次发动三角攻击。保持无输入会触发原版抱摔
受击与起身流程，Gene 剩余 77 HP。Direct3D 回放也呈现了抱摔与屏幕扭曲效果。
另一份测试档使用原版新游戏的默认装备，一次三角键
连中两段，每段扣 12 点，头目血量从 1,070 降到 1,046 HP。

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
- 当前回归构建通过 390 项原生检查，包括静音设备音频和影片解码。
  先前回归也完成了两项可选的独立 ADX 样本对比。受限 GPU 像素与独立 CPU 方程一致。
  绘制保留原 OT 的层级、
  深度、同桶后进先出和上一帧提交顺序。
- 同一DLL中的两个RNG对10272个种子完成20544次状态／浮点位精确比较，依据实际指令的受限模型验证；
  这不是 PS2 实机验证。上游匹配比例也不是本项目的移植完成比例。

R103 已运行原版增援并进入双头目战。手柄回放验证了强化启动、同时命中两名头目
以及强化耗尽后恢复普通行动，Direct3D 11 回放显示了光效出现和消退。受控测试
在最后一击前分别将两名头目的 HP 设为 12，再用实际攻击验证两次击败登记、原版胜利
标志与掉落。录像随后用手柄走到原版出口并按叉键，Direct3D 11 共呈现 587 帧。
淡入结束后，R102 显示角色、走廊和 HUD，Gene 剩余 90 HP，渲染器、GPU 和窗口
均无报错。另一段完整初始血量的录像验证了原版蓄力夹击，以及从背后用三角键命中头目时
的受击动作和脚步声。

合并构建验证了 R100 西区原版战斗登记的八次击倒，Gene 当时剩余 1 HP。R103 的另一条
手柄回放从两名头目各有 597 HP 开始。后＋圆圈及击飞碰撞分别将其打到 572 和 587 HP。
第一发轮盘冲击波各造成 80 点伤害。第二发冲击波及其击飞碰撞后，两名头目分别剩余
412 和 487 HP，Gene 在第 9,000 帧仍然存活。Direct3D 呈现了前两招的 462 帧，
渲染器无报错。完整血量的双头目实战仍在验证。

下一条固定手柄回放在两发冲击波之后启动强化。强化耗尽时命中两名头目，随后三角键
对两名头目各命中两次。第 9,650 帧两名头目剩余 450 和 383 HP，Gene 剩余 7 HP。
另一条西区固定回放到第 10,208 帧已登记九次原版击倒，Gene 剩余 1 HP。
当前西区出口条件要求十二次击倒。

Direct3D 11 已能从启动前准备的原版格式 R104 测试存档呈现原版 EV05 过场。
淡入结束后，首个镜头显示 Gene 面向头目。固定回放呈现到编号 9 的镜头，渲染器、
GPU、特效和窗口均无报错。更长的 Direct3D 回放验证了原版英文字幕。原生 CPU 回放播完
EV05，进入 R104 原版头目初始化。

另一条 Windows 回放从预置三张钥匙卡与测试落点的 R101 原版格式存档开始。固定手柄输入
打开原版门，在保存地图选择「继续」并进入 R104。Direct3D 在本次回放中呈现 1,170 个
场景帧，包含 R104 的头目过场，图形相关报错为零。用同一份 R101 测试档延长回放，过场
结束后左摇杆能通过原版行走状态移动 Gene。另一条从 R104 测试档开始的 Direct3D 回放在
淡出结束后呈现了头目场景里的行走。R103 双头目战也验证了两次前闪成功保住血量，获得
Heat 和段位分。实战收齐三张卡和完成头目战仍在开发。

### 构建与运行

按上方英文段落的准备、构建和运行命令操作，替换为自己的 ISO、原生存档目录及
FFmpeg 路径。公共仓库不含游戏程序/资源、BIOS、私有 SDK、工具二进制或密钥。
Windows 窗口目标还需要 Windows SDK 的 x64 图形库、D3D11 硬件和音频设备。
窗口、模型与 UI 渲染器在同一个进程中使用同一块显卡。默认优先选支持 D3D11 功能级别 11.0
的 NVIDIA 显卡，也可回退至满足要求的其他显卡。设置 `GODHAND_D3D11_ADAPTER_INDEX` 可以按
DXGI 索引显式选卡。启动日志会记录厂商、设备编号和 LUID。
标准 Windows 程序接受含中文等 Unicode 字符的 ISO、存档和 FFmpeg 路径。

窗口默认隐藏，`--show` 才显示；`--muted` 仅静音本程序，不改系统音量。原生存档
目录不存在时走原版无卡分支。标题读档已验证恢复 R100 测试档。游戏内存档和完整流程仍在开发。
Windows 存档后端在槽目录中暂存各文件，成功关闭后提交。执行
`python -B tools/build-native.py --focus mc-file-save` 可在无需 ISO 的情况下验证写档和恢复。
WASD 移动，方向键对应十字键。Z 对应叉键，X 对应圆圈，C 对应方块，Esc 对应三角，
Enter 对应 Start。确认和取消由原版 ELF 映射表决定。
加上 `--trace-native-input` 可在标准错误流记录已接收的输入、更新后的菜单与场景状态，以及
实际呈现的场景帧累计数。
R100 手柄回放工具兼容八列和十列记录，十列格式保留模拟扳机。

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
要复现读档路线，用空白存档目录运行 CPU QA，并在参数末尾加 `--r102-save` 写出存档。
将这个存档目录的副本传给 `startup-qa`，使用 `--qa-load --qa-r102-walk` 和
`--qa-capture FILE --qa-capture-count 36`。游戏会经原版标题菜单读取存档。

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
