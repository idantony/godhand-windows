# Verified progress

This is an early source port, **not a playable Windows game**. The chosen route
is readable recovery of original game code with native platform adaptation.
See [progress.json](progress.json) for the latest verified, machine-readable state.

## West route from New Game to R105

An internal native CPU integration run replays the controller recording from
New Game through the West encounter, all 28 R003 EV02 cuts and entry into R105.
It matches 1,750 archived startup records and 14,257 archived gameplay records.
The event clears at tick 20,046, R105 loads at 20,053, and an unfinished enemy
attack branch stops the run at 20,265.

Direct3D shows the first R003 cut on the original West route. A separate run
starts at R003, plays all 28 cuts and verifies the visible R105 window by
reading it back after presentation.

内部原生 CPU 集成回放从新游戏出发，通过手柄录像完成西区战斗、R003 的 EV02 全部 28 个镜头，
随后进入 R105。回放与归档中的 1,750 条启动记录和 14,257 条游戏记录一致。事件在第 20,046 个
tick 结束，第 20,053 个 tick 进入 R105，第 20,265 个 tick 停在敌人一个尚未覆盖的攻击分支。

Direct3D 回放验证了原西区路线上的 R003 首个镜头。另一轮测试从 R003 开始，播完全部 28 个
镜头后进入 R105，并在画面提交后读回像素，验证了实际窗口。

## R104 victory event and R021 Stage Clear

The R104 victory replay now presents the original Stage Clear screen in R021.
The test lowers the boss to 60 HP immediately before an original 80-point
Shockwave. Native code applies the hit, plays all 14 EV06 shots and releases
the arena before entering R021. Direct3D presents the clear time, Continue,
villager and enemy tallies through the planned stop at tick 4,710. A separate
CPU replay verifies all 4,712 controller samples and reaches the original total
payout routine. A full-health boss victory and the complete confirmation and
save flow remain in development.

The Direct3D replay presents 725 scene frames overall. Captured R021 frames at
4,615, 4,650 and 4,705 show the original result interface and its tally
animation. The renderer, world GPU, effect post-processing and window each
report zero errors through tick 4,710.

R104 的胜利回放已在 R021 呈现原版结算画面。测试在原版 80 点冲击波命中前将头目血量
调到 60 HP。原生程序处理命中，播放 EV06 全部 14 个镜头，清理竞技场并进入 R021。
Direct3D 呈现结算时间、继续次数、村民和敌人击杀统计，到第 4,710 帧按计划结束。
独立的 CPU 回放核对全部 4,712 个手柄采样，并到达原版总奖金流程。满血头目战的完整
胜利路线、确认和保存流程仍在开发。

## R104 hold, escape and New Game equipment

Two native CPU replays continue the original R104 boss fight after Retry.
Alternating Cross and Triangle triggers the paired escape at tick 4,018.
Gene returns to his normal stance at tick 4,104 with 100 HP. A subsequent
left-stick interval moves him and Triangle starts another attack. The replay
runs through tick 4,250 with every controller sample verified.

With no escape input, the original A8:19 distortion and debris run during
the hold. Gene takes 23 HP of damage at tick 4,108 and stands again at tick
4,292 with 77 HP. This replay verifies all 4,350 samples. A separate Direct3D
replay shows Gene being held at tick 3,252 and the distortion effect at 3,256,
when his HP has dropped to 77. It presents 2,967 scene frames through tick
3,280 with zero renderer or window errors.

A separate original-format test save uses the authored New Game equipment
and health scaling. One Triangle press at tick 2,627 produces two 12-point
hits at ticks 2,656 and 2,659. The boss drops from 1,070 to 1,046 HP. The
replay reaches tick 3,100 with every applied sample verified.

R104 重试后的两条原生 CPU 回放验证了头目的抱摔。交替按叉键和三角键后，第 4,018 帧
触发挣脱。Gene 在第 4,104 帧以 100 HP 回到站立，随后能用左摇杆移动，并再次发动
三角攻击。回放逐条核对了 4,250 个手柄采样。保持无输入时，会生成原版抱摔特效包，
并在第 4,108 帧扣除 23 HP，Gene 于第 4,292 帧以 77 HP 站起。第二条回放核对了
全部 4,350 个采样。独立的 Direct3D 回放在第 3,252 帧呈现抱摔，第 3,256 帧
呈现画面扭曲，Gene 已降到 77 HP。回放到第 3,280 帧共呈现 2,967 个场景帧，
渲染和窗口均无报错。

使用原版新游戏默认装备的独立测试档中，第 2,627 帧按一次三角键，在第 2,656 帧和
第 2,659 帧各造成 12 点伤害，头目血量由 1,070 降到 1,046 HP。3,100 个手柄采样逐条一致。

## R104 Retry and ten R100 west defeats

An original-format three-card R101 test save reaches R104 through the keycard
gate. Recorded inputs exercise the boss's projectile, melee attacks, and Gene's
lethal fall. The original Retry menu becomes actionable at tick 3,728. The
recorded Cross input increments the Continue count, retires the old room and
reloads the checkpoint. The replay returns to the fight with Gene at 100 HP
and the boss at 1,000 HP. All 4,002 applied controller samples match the tape.
A separate Direct3D replay presents the second hit on the boss with 991 HP.

The fixed R100 west replay earns and activates Heat through the original
controller input. The tenth defeat is recorded at tick 12,656 with Gene at
12 HP. The original gate requires twelve defeats.

一条原版格式的 R101 三钥匙卡测试档回放经钥匙卡门进入 R104，验证头目的发射、
近身攻击和 Gene 的死亡动作。原版重试菜单在第 3,728 帧可以操作。回放中的叉键
增加继续次数，触发旧场景清理，再加载检查点。Gene 以 100 HP 回到战斗，头目
重新加载为 1,000 HP。4,002 条实际读取的手柄采样与回放一致。另一条 Direct3D
回放呈现了第二次命中，头目血量降到 991。

R100 西区的固定回放用原版手柄输入积攒并激活热血。第 12,656 帧记入第十次击倒，
此时 Gene 保有 12 HP。原版门需要十二次击倒。

## R104 boss combat and R101 side arena

Direct3D presents the boss taking a Triangle hit after R104's original intro.
In a separate CPU replay, the original-format three-card R101 test save reaches
R104 through JMP1 and the save map. The boss launches its first projectile at
tick 3,097. The native collision and damage handlers take Gene from 100 to 80 HP
at tick 3,108, play his original fall and retire the projectile and its effects.
All 3,156 applied input samples match the tape. An otherwise matching run with
a right-stick input at tick 3,100 evades the projectile and keeps Gene at 100 HP.

The zero-card R101 save reaches the side arena through the original JMP2
transition. Direct3D shows the first attack and the enemy's reduced HP bar.
The longer CPU replay keeps Gene at 100 HP and reduces the two active enemies
from 74 HP each to 26 and 71.

The same revision also reproduces four original forward-evade rewards in R103.
All 9,900 input samples match and Gene retains 25 HP at each reward.

Direct3D 已呈现 R104 原版过场后的三角键命中。另一条 CPU 回放从有三张钥匙卡的 R101
原版格式测试档出发，经 JMP1 和保存地图进入 R104。头目在第 3,097 帧发射普通弹，原版
碰撞和受伤流程在第 3,108 帧让 Gene 从 100 HP 降至 80 HP，并播放击倒动作、清理发射物
和附着特效。3,156 条实际读取的手柄采样与回放逐条一致。对照回放在第 3,100 帧加入右摇杆
侧闪，Gene 保持 100 HP。

未编辑的 R101 零钥匙卡存档也走通了原版 JMP2 转场。Direct3D 画面可见第一次攻击和敌方
血条下降。较长的 CPU 回放中，Gene 保持 100 HP，两名出场敌人从各 74 HP 降到 26 和 71。

同一版本复现了 R103 的四次原版前闪奖励。9,900 条手柄采样逐条匹配，每次获得奖励时
Gene 都保有 25 HP。

## R104 post-intro movement and R103 forward evade

The original-format R101 test save contains three keycards and a door-side
landing position. One native replay opens the original gate, enters R104,
finishes EV05, and moves Gene with recorded left-stick input. All 3,041
applied controller samples match the tape. At tick 3,041 Gene has 100 HP
and is in the original walk state. The boss has 1,000 HP and attempts its
next distinct attack. A separate Direct3D replay from the R104 test save
shows the same walk state after the fade clears. The earlier caption
capture remains pixel-identical, and the Direct3D replay reports zero
graphics errors through its deliberate stop at tick 2,606.
The extended Direct3D replay from the R101 test save presents 2,709 scene
frames before an unported EM03 move at tick 3,022.

In the R103 twin-boss fight, forward evades at ticks 9,496 and 9,546
preserve Gene's 25 HP and award original Heat and level points. The first
adds 1.5 Heat and raises level points from 582 to 657. The control run
without the first evade drops to 16 HP and has no evade award. The
two-evade run eventually ends in defeat.

R101 原版格式测试档预置三张钥匙卡与门边落点。一条原生回放打开原版门，进入 R104，
播完 EV05，并用左摇杆让 Gene 走动。实际读取的 3,041 条手柄采样逐条匹配。第 3,041 帧时
Gene 保持 100 HP，处于原版行走状态。头目有 1,000 HP，准备下一招。另一条从 R104
测试档开始的 Direct3D 回放呈现了淡出结束后的行走，字幕画面与此前逐像素相同，并且
到第 2,606 帧主动停止时，所有图形错误计数为零。
R101 测试档的跨房间 Direct3D 延长回放呈现了 2,709 个场景帧，在第 3,022 帧遇到一段待恢复的 EM03 动作。

R103 双头目战里，第 9,496 和 9,546 帧的前闪都保住了 Gene 的 25 HP，并获得原版 Heat
和段位分。第一次增加 1.5 点 Heat，段位分从 582 升到 657。省略第一次前闪的对照回放中
Gene 掉到 16 HP，未触发奖励。两次前闪的回放最终战败。

## R101 gate and Windows R104 handoff

The original-format R101 test save contains three keycards and a landing
position prepared before launch. The recorded controller opens JMP1 and
selects Continue on the save map. The port retires the original R101 objects
and loads R104. Direct3D plays the R104 boss intro by tick 1,475 and presents
1,170 scene frames across the run with zero renderer, GPU, effect or window
errors. The matching native CPU run reaches R104 tick 1,950. Another native
replay verifies complete EV05 playback and reaches the original boss setup.
Collecting all three cards during play remains a verification point.

R101 原版格式测试档在启动前预置了三张钥匙卡和测试落点。固定手柄输入打开 JMP1，
在保存地图选择「继续」。移植版清理原版 R101 物件并载入 R104。到第 1,475 帧，
Direct3D 正在播放 R104 头目过场，整个回放呈现 1,170 个场景帧，渲染器、GPU、
特效和窗口均无报错。同一条原生 CPU 回放在 R104 运行到第 1,950 帧。另一条原生
回放播完 EV05，进入头目初始化。实战获取三张卡仍在验证。

## R104 intro presentation and later combat

The Direct3D 11 replay loads an original-format R104 test save prepared
before launch. The original fade clears at tick 607, and camera cut 9 is
still playing at tick 1,630. The graphics replay presents 1,326 scene
frames across the run with zero renderer, GPU, effect and window errors.
The CPU replay enters original EV05 cut 10 at tick 1,664. Player control
after the intro and the natural R101 gate into R104 remain in development.

The R103 replay starts with both bosses at 597 HP. After two Shockwaves,
Heat activates and its expiry hits both bosses. A Triangle follow-up
hits each twice. The run reaches tick 9,650 with Gene at 7 HP and the
bosses at 450 and 383 HP. The fixed R100 West route independently shows
nine credited defeats by tick 10,208, with Gene alive at 1 HP. The active
gate requires twelve defeats.

Direct3D 11 回放从标题读取启动前准备的 R104 原版格式测试存档。第 607 帧淡入结束，
到第 1,630 帧仍在播放原版编号 9 的镜头。整个回放共呈现 1,326 个场景帧，渲染器、
GPU、特效和窗口均无报错。CPU 回放在第 1,664 帧进入 EV05 编号 10 的镜头。
过场后的玩家操作与从 R101 自然进入 R104 仍在开发。

R103 回放从两名 597 HP 的头目开始。两发冲击波之后启动强化，强化耗尽时命中两名头目。
随后的三角键各命中两次。回放到第 9,650 帧，Gene 剩余 7 HP，两名头目分别剩余
450 和 383 HP。独立的 R100 西区固定回放在第 10,208 帧已经登记九次击倒，Gene 仍有
1 HP。当前出口条件要求十二次击倒。

## Joint R100 west and R103 combat

The merged source passes the fixed original R100 west route and records an
eighth defeat at tick 9,296 with Gene alive at 1 HP. The run reaches tick
9,775 without an unsupported call. R103 raw controller input starts with two
full-HP bosses, lands Back + Circle, and uses the original wheel to select
Shockwave twice. The later replay reaches tick 9,000 with Gene at 2 HP and the
bosses at 412 and 487. A separate Direct3D replay presents the first two moves
over 462 frames. The existing controlled R103 victory-return proof still
presents R102 at scene 8,241 on the merged runtime.

合并源码通过了 R100 西区固定手柄回放，第 9,296 帧登记第八次击倒，Gene 剩余 1 HP。
回放到第 9,775 帧没有遇到未实现调用。R103 的原始手柄回放从两名完整血量的头目开始，
命中后＋圆圈，再从原版轮盘两次选中冲击波。到第 9,000 帧，Gene 剩余 2 HP，两名头目
各剩余 412 和 487 HP。独立 Direct3D 回放呈现了前两招的 462 帧。合并构建也通过了
受控的 R103 胜利返回验证，在第 8,241 个场景帧显示 R102。

## New Game Retry and original arena combat

On the east route from New Game, the recorded controller input wins the
opening fight, breaks the original door and reaches an armed enemy. Its
OM363 hit drops Gene from 1 HP to zero. The Game Over menu accepts Cross
at tick 5063. Continue advances from zero to one and R100 reloads with
Gene at 100 HP. A Windows Direct3D replay presents the original menu
and the reloaded arena. A west route separately reaches the original
reinforcement area with the Devil encounter active.

An unchanged zero-defeat R102 card yields the first of the six qualifying
defeats at tick 2804. Original effect retirement clears the defeated actor.
The same replay damages a second enemy, escapes an EM0A grab and continues
to 2,920 native R102 room frames. The remaining gate condition is five
further qualifying defeats.

A controller replay through the original R102 door uses the gate-ready
fixture and reaches 1,800 R103 room frames. Ten face-button edges drive
the original escape counter and Gene returns to neutral. The direct
R103 fight replay drops the first attacker on its second successful
counter. The original second wave enters and lands a hit on Gene. Direct3D presents 1,623
consecutive R103 frames on the original door route. The escape counter
appears at scene 2743, followed by 0.167230 m of observed movement after
stick input at scene 2921. The renderer, world GPU, effects and window
report zero errors.

从新游戏开始的东侧回放赢下开场战斗、打破原版门，随后 Gene 被持械敌人击倒。在第 5063 tick
按叉确认重试，原版继续次数从零增加到一，Gene 以 100 HP 回到 R100。Windows Direct3D 回放
呈现了原版重试菜单和重载后的关卡。另一条西侧路线触发了原版恶魔遭遇，并抵达增援区域。

R102 从零击败存档开始，于第 2804 tick 记录六个所需击败中的第一个。原版流程清理已击败的敌人。
同一回放打伤第二个敌人、挣脱 EM0A 擒抱并运行到 2,920 个 R102 原生房间帧。开门还需五个有效击败。

预置开门条件的测试存档可用手柄走过 R102 原版门，进入 R103 并跑到第 1,800 个房间帧。
十次按键触发原版挣脱反击，Gene 随后回到待机。直接进入 R103 的战斗回放验证第二次反击击倒敌人，
原版第二波入场并命中 Gene。Direct3D 在原版门路线连续呈现 1,623 个 R103 帧，涵盖第 2743
场景帧的挣脱反击和第 2921 场景帧由摇杆输入带来的 0.167230 米位移。渲染器、场景 GPU、
特效和窗口无报错。

## Later R102 combat and mid-room gate

An original save and mapped controller input reach a later R102 encounter.
The hidden, muted Windows run presents 88 consecutive Direct3D frames. The
attack starts at tick 1874, Gene drops from 45 to 36 HP at tick 1907, and the
enemy begins its original recovery at tick 1938. The 64 captured pairs of
pre-UI and composited frames match the earlier independent build byte for
byte. The renderer, GPU and window report zero errors.

A separate title-load fixture supplies the original six-defeat count and a
qualifying defeated enemy. The original script creates five reinforcements,
ends the camera event, clears gate collision, moves the panels and retires
the gate object. The integrated CPU replay completes 120 room frames on
this fixture and reaches 1,113 R102 room frames in the New Game regression.

原版存档和手柄回放进入 R102 后续交战。Windows 隐藏、静音测试呈现 88 个连续的 Direct3D 帧。
敌人在第 1874 tick 出招，第 1907 tick 命中，Gene 从 45 HP 降到 36 HP。第 1938 tick 敌人
进入原版收招。64 组场景和最终合成图与早期独立构建逐帧一致，渲染、GPU 和窗口均无报错。

独立读档测试预置了六次击败计数和一名符合条件的已击败敌人。原版脚本生成五个增援，结束镜头，
解除门碰撞，移动门扇并清理门物件。合并版本通过了 120 个房间帧的门测试，新游戏 CPU 回放则
推进了 1,113 个 R102 房间帧。

## R102 save, reload and movement on Windows

The original Event 04 save prompt writes the R102 checkpoint to a fresh
native card profile and reads it back through the original verification
phase. A separate Direct3D process selects the saved slot at the title
screen and confirms Continue. It presents 36 consecutive R102 frames and
records 27 frames with position changes around 24 forward stick samples.
The player remains at 100 HP. The renderer, GPU, effects and window report
zero errors.

EV04 结尾的原版保存菜单将 R102 存档写入空白记忆卡目录，并经过原版回读校验。
独立启动的 Direct3D 回放在标题菜单选择存档，再确认继续。R102 连续呈现 36 帧，
其中 27 帧的玩家位置发生变化，回放共输入 24 次向前摇杆。角色生命值保持 100。
渲染、GPU、特效和窗口均无报错。

The same production source passes the original R100 Retry and R101 box replays
in the CPU scheduler. R100 accepts Cross from the original Game Over menu,
reloads Gene with 100 HP and advances another 120 frames. In R101, Circle
lifts the box and Square throws it. The native game shatters the box, awards
200 G when Gene collects the drop, plays its pickup sound, and releases the
box and item.

同一份生产源码也通过了 R100 重试和 R101 箱子的 CPU 回放。R100 在原版 Game Over 菜单
接受叉键，重载 Gene 的 100 点生命值，再推进 120 帧。R101 以圆圈键举箱、方块键投掷，
游戏处理箱子碎裂、掉落与拾取，增加 200 金币，播放拾取音效并释放箱子和掉落物。

## First R102 frames presented on Windows

The controlled original-disc replay selects Continue after R002 Event 04.
R102 initializes its stage, player, enemies and room objects. Direct3D presents
16 consecutive R102 frames with the HUD, and the run captures the matching
pre-UI world and final composite. The renderer, effects, GPU and window report
zero errors. The next original operation is an enemy attached sound request.
R102 movement and combat remain to be verified.

受控实盘回放在 R002 的 EV04 播完后选择继续。R102 初始化原版场景、玩家、敌人和房间物件。
Direct3D 连续呈现 16 帧，并保存对应的 HUD 合成画面和合成前的场景画面。渲染、特效、GPU
和窗口均无报错。下一处待恢复调用是敌人的附属音效，R102 的移动和战斗仍待验证。

## Complete R002 Event 04 on Windows and enter R102 initialization

The controlled Windows replay presents 3,390 consecutive frames in R002 and
starts and completes all 24 shots of Event 04. It finishes the original
event cleanup, presents the original save prompt, and selects Continue
using D-pad Down and Cross. The game enters R102 initialization at tick 6823
and reaches the original positional sound table. The renderer, effect layer,
GPU, and window report zero errors.

受控 Windows 回放在 R002 连续呈现了 3,390 帧，完整播放 EV04 的 24 个镜头并执行片尾清理。
画面显示原版保存界面，回放用方向键下和叉键选择继续。第 6823 tick 进入 R102 初始化，推进到
原版声音定位表。渲染、特效、GPU 和窗口均无报错，R102 的首帧游戏画面仍待验证。

## Four R002 cutscene shots presented on Windows

A controlled Windows run takes the original R105 exit and presents 725
consecutive R002 frames. It completes shots 1–4 of Event 04 and captures
16 final and pre-UI world frames from shot 2. The renderer, GPU, and window
report zero errors. The run stops at the next original effect callback.

A separate original-disc CPU replay completes shots 1–20 and reaches shot 21
at tick 6250. It exercises original camera roll, lighting, prop and character
updates, and effect trails with up to 29 segments. It stops at the next
original effect update. In a separate controlled R105 battle, the native
logic runs the original EM27 attack, attached sound, and Gene's minor hit
reaction. The replay reaches a new enemy attack decision at tick 4048.

受控 Windows 回放沿 R105 原版出口进入 R002，连续呈现 725 帧，播完 EV04 前四个镜头。
第二个镜头保留了 16 组最终画面和 UI 合成前的世界层画面。渲染、GPU 和窗口无报错，
回放在下一处原版特效回调停下。

另一轮原盘 CPU 回放经过原版相机横滚、灯光、场景物件、角色和最多 29 段的拖尾特效，
走完前 20 个镜头，在第 6250 tick 进入第 21 个镜头，并停在下一处原版特效更新。
独立的 R105 受控战斗回放跑过了原版 EM27 攻击、附着音效和 Gene 的小受击动作，
在第 4048 tick 到达下一处敌人攻击选择。

## First R002 cutscene shot presented on Windows

The Windows replay enters R002 through the original R105 exit. Its controlled
setup supplies the existing open-gate save bit and places Gene at the original
exit, where the replay presses Circle. The event initializes all 17 original
actors. The Direct3D renderer uploads the original 1024×128 sprite atlas and
presents 102 R002 frames, with 16 captured frames from Event 04's first shot.
The replay reaches the beginning of the second shot at tick 3522 and stops
at original actor update `0x002DF860`. Renderer and window errors remain zero.

A separate disc replay uses recorded controller samples from New Game to
reach R100's AT10 area with Gene alive at tick 4730. The original area records
the hit. The battle counter is 2, and the normal door flow is still in development.

Windows 回放从 R105 原版出口进入 R002。受控测试给定原版已开门存档位和出口处的 Gene 位置，
回放按圆圈触发切换。过场初始化 17 个原版角色。Direct3D 上传原盘的 1024×128 特效贴图，
呈现 R002 的 102 帧，从 EV04 首个镜头抓取 16 帧。回放到第 3522 tick，进入第二个镜头后
在原版角色更新 `0x002DF860` 停下。渲染和窗口均无报错。

另一轮原盘回放从新游戏开始，用录制的原版手柄输入到达 R100 的 AT10 区域。第 4730 tick
记录到原版区域触发，Gene 仍然存活。战斗计数为 2，正常门流程仍在开发。

## R101 gate on Windows and R002 event initialization

The Windows title-load replay confirms the chapter map and uses original mapped
pad input to open the R101 gate and enter the corridor. The bounded replay
finishes at tick 835 with 222 player updates and no renderer or window errors.

In an independent native CPU replay, the original R105 exit receives Circle
from a controlled saved-open state, with Gene placed at the authored exit.
The real gate voice is queued on the native audio service. The original
transition loads R002 and starts reserved Event 04, whose disc audio and
camera track initialize. This earlier replay reaches the original actor load
callback at event overlay offset `0x04D0`.

Windows 从原版章节地图读档，按原版手柄映射打开 R101 的门并走进门后走廊。回放在预定的
第 835 tick 结束，完成 222 次玩家更新，渲染与窗口均无报错。

另一条原盘无画面回放从 R105 已开门的存档状态开始，将 Gene 放在原版出口并按圆圈。原生
音频服务接收开门声，原版房间切换载入 R002 并启动预定的 EV04。过场音频和镜头轨道完成
初始化，这轮较早的回放推进到过场模块原版角色载入回调 `0x04D0`。

## R100 post-battle event and saved R101 presented on Windows

The recorded New Game route presents 500 more native frames after both R100
opening enemies die. At scene 4232, Gene is alive with 1 HP, and the original
post-battle event has run. The test stops at tick 4372. A longer recorded
route presents 900 post-victory frames and reaches the original paired grab
escape at scene 4591. It exits at tick 4772 with Gene alive at 1 HP. The
normal room exit remains in development.

An isolated Windows title-load test confirms the original map with Cross.
The map uses the original clear depth, with sprite Z0 and glyph Z65535 carried
through the Fade pass. Scene 461 presents the loaded R101 with its HUD. At
the planned 800-tick limit the native run reports 187 player updates and
zero runtime, renderer, GPU or window errors.

Windows 用同一套原版输入打赢 R100 开场两名敌人后，又呈现 500 帧并执行原版战后事件。
第 4232 帧时 Gene 仍剩 1 HP，测试在第 4372 tick 结束。更长的回放在首战后呈现 900 帧，
在第 4591 帧进入原版抓技的挣脱动画，在第 4772 tick 正常结束，Gene 仍剩 1 HP。
房间原版出口仍在开发。

Windows 读档测试从原版章节地图按叉键确认。地图淡入沿用原版深度清除值，精灵深度为 0，
文字深度为 65535。第 461 帧呈现读档后的 R101 和 HUD。回放到预定的 800 tick 上限，
完成了 187 次玩家更新，运行、渲染、GPU 和窗口均无报错。

## Both R100 opening enemies defeated on Windows

The hidden, muted Windows New Game replay uses a recorded pad route and sends
the original difficulty confirmation on tick 312. This aligns startup timing
with the native CPU replay, including the original per-frame random draws.
The 2,125 shared player updates match for raw pad input, actor state, HP, and
positions. Both authored enemy deaths are recorded in the original room save,
and Gene has 1 HP. Windows presents scene 3732 and exits at tick 3872 with zero
renderer or runtime errors. The CPU replay continues for 500 game frames.
The normal exit from R100 is still in development.

An independent native CPU title-load replay confirms the map in an original-format
R101 test save. It completes 175 consecutive R101 scene frames through native
packet collection. A second replay uses original left-stick movement and a
Square attack.

隐藏、静音的 Windows 新游戏回放通过录制的原版手柄输入打赢首战。回放在第 312 tick
发送原版难度确认，与无画面回放对齐启动时机和原版随机数推进。两条回放的 2,125 次
玩家更新在输入、角色状态、血量和位置上一致。原版房间存档记录两名敌人的死亡，Gene
剩 1 HP。Windows 呈现第 3732 帧，在第 3872 tick 正常退出，运行和渲染均无报错。
无画面回放在胜利后又运行 500 帧，R100 原版房间出口仍在开发。

独立的无画面回放在原版章节地图确认 R101 测试档，连续完成了 175 帧原版场景的绘制指令。
另一轮验证了原版左摇杆移动和方块攻击。

## R105 Game Over and same-room Continue on Windows

A hidden, muted Windows replay selects R003 at the title handoff. It presents
all 28 original EV02 cuts and follows the room exit into R105. The original
Game Over menu is presented at scene 4399. Cross confirms Continue and advances
the continue counter from 0 to 1. Original GameMain State 5 reloads R105.
The first reloaded scene, 4491, is presented with Gene at 100 HP. The run exits
successfully at tick 4612. A separate native CPU replay completes 60 game frames
after the reload.

A recorded original pad route defeats both opening R100 enemies with Gene at
1 HP on the native CPU replay. The original room save records both deaths.
The later room-exit gate and Windows victory presentation were pending at this checkpoint.

隐藏、静音的 Windows 回放在标题交接处选入 R003，呈现 EV02 全部 28 个镜头，随后沿原版出口
进入 R105。第 4399 帧呈现原版 Game Over 菜单。按叉键确认继续，继续次数由 0 增至 1，原版
GameMain State 5 重载 R105。重载后的第一帧为 4491，Gene 恢复到 100 HP。回放在第 4612 tick
正常退出。另一次无画面的原盘回放在重载后完成了 60 帧。

无画面的 R100 手柄回放击败了开场的两名敌人，Gene 剩 1 HP。原版房间存档记录两名敌人的死亡。
这个节点当时仍待验证后续房间出口和 Windows 胜利画面。

## First tutorial presented and dismissed on Windows

A Windows New Game replay approaches the first R100 tutorial using the
original stick input. Circle opens the original tutorial, and frame 1655
presents its plaque, text and right-stick icon. Cross dismisses it, and frame
1682 presents the closed state. The merged tutorial and combat sources pass
371 native checks across 381 C modules. Eighteen opt-in checks were skipped.

A separate Windows pad replay shows the original escape counter. Stamp20
collides with stamp19 and the original contact reduces stamp19's HP from 26
to 16. The controlled R003 route presents 420 R105 frames, through Gene's
first two original damage reactions. The first encounter and its normal
room exit remain in development.

Windows 新游戏回放通过原版摇杆输入走到 R100 的第一个教程，按圆圈打开面板。第 1655 帧显示
原版提示框、文字和右摇杆图标。按叉键关闭后，第 1682 帧显示关闭状态。合并后的教程与战斗源码
编译 381 个 C 模块，通过 371 项原生检查，本次跳过 18 项需额外设备或工具的检查。

另一轮 Windows 手柄回放呈现了原版挣脱反击。被甩出的敌人撞到第一名对手，原版碰撞伤害把对手的
HP 从 26 降到 16。R003 受控路线进入 R105 后呈现了 420 帧，包括 Gene 的前两次原版受击反应。
完整首战及其正常房间出口仍在开发。

## First encounter grab and R105 enemy movement

The hidden, muted New Game replay presents 2,606 scene frames through Gene's
original grab and left-stick escape prompt. It also runs the original OM305
destructor. The next operation requests an attached CORE sound at
`0x002CBF78`. The first fight and its normal exit remain in development.

The controlled Windows route follows all 28 EV02 cuts into R105 and presents
183 R105 scene frames. The next operation creates an attached effect at
`0x00306528`. Both Windows runs report zero renderer, GPU, postprocess and
window errors. The combined source compiles 381 C modules and passes 371
native checks. Eighteen checks requiring additional tools or devices were skipped.

隐藏、静音的新游戏回放呈现 2,606 个场景帧，包括 Gene 的原版抓技和左摇杆挣脱提示，
并完成 OM305 的原版销毁流程。下一处调用是附着音效 `0x002CBF78`。完整首战及其正常出口仍在开发。

Windows 受控路线播完 EV02 的 28 个镜头后进入 R105，呈现 183 个 R105 场景帧。
下一处调用是附着特效 `0x00306528`。两次实跑的渲染、GPU、后处理和窗口错误均为 0。
合并源码编译 381 个 C 模块，通过 371 项原生检查，本次跳过 18 项需额外工具或设备的检查。

## All 28 R003 EV02 cuts presented on Windows

A hidden, muted Windows run uses the original A38 destination at title
handoff to isolate R003. It presents 2,785 R003 frames across all 28 EV02
cuts. Renderer, GPU, postprocess and window error counts are zero. Original
GameMain State5 selects R105 and reaches the Flag object's retirement at
`0x00312708` in R003. The regular Windows executable also builds. The frozen
native regression suite passes 366 checks across 379 C modules, with 18
device- or tool-dependent checks skipped.

A separate New Game run reaches 1,995 presented R100 scene frames, with
Gene's HP falling from 100 to 91, 81 and 78. The next enemy operation is
`0x00213EA8`. The full combat encounter, player-driven transition into R003,
and loading R105 after EV02 remain in development.

隐藏、静音的 Windows 回放在标题交接时通过原版 A38 目的地进入 R003，呈现 EV02 的
全部 28 个镜头，共 2,785 帧。渲染器、GPU、后处理和窗口错误数均为 0。原版
GameMain State5 选定 R105，停在 R003 的 Flag 对象清理 `0x00312708`。普通 Windows
可执行文件也已编译。冻结版编译 379 个 C 模块，通过 366 项原生检查，本次跳过
18 项需额外设备或工具的检查。

另一轮新游戏回放在 R100 呈现 1,995 个场景帧，Gene 的 HP 从 100 依次降到 91、81、78，
停在下一处敌人行为 `0x00213EA8`。完整首战、由玩家操作进入 R003，以及 EV02 后加载 R105
仍在开发。

## Two-way combat and a controlled R003 transition

A Windows New Game replay presents 1,816 original scene frames. Two Triangle
hits take the first enemy from HP 74 to 50. Enemy attacks take Gene from HP
100 to 91 and then to 81, with the original hit reaction and knockdown visible
in frames 1,797 and 1,812. The next enemy behavior is `0x0023C620`.

A separate packet test supplies the RoomSave and trigger prerequisites, uses
the original Door Continue and room teardown, and completes the first R003
frame. The native regression build passes 365 checks. The Windows route from
the start of the encounter through the door remains under development.

Windows 新游戏回放呈现 1,816 个原版场景帧。两次三角攻击把首名敌人的 HP 从 74 打到 50。
Gene 两次受击后 HP 从 100 降到 91，再降到 81。第 1,797 和 1,812 帧呈现原版受击和击倒。
下一处敌人行为是 `0x0023C620`。

独立测试设置 RoomSave 和触发点前置条件，走完原版 Door Continue 和旧房间清理，完成 R003
首帧。原生回归构建通过 365 项检查。从首战开始到开门的完整 Windows 流程仍在开发。

## Title Load restores and presents R100 on Windows

A hidden, muted Windows run restores slot 3 from a synthetic profile encoded
in the original save format. It completes the card read and checksum, exits
the title Load task, reloads the original player model and motions, and
presents R100 with Gene and the HUD at scene frame 1,634. The run reaches
tick 1,826 and 1,501 scene presents with 1,200,273 world packets. Renderer,
GPU, postprocess and window error counters are all zero. The next original
enemy Move is `0x002147D0`.

A separate New Game run reuses the same executable and presents the original
3-point player hit again. It reaches tick 1,810 and 1,669 scene presents.
The next original enemy scenery reaction is `0x0020B394`. The frozen
source builds 370 C modules and passes 351 native checks.

隐藏、静音的 Windows 实跑从原版格式的合成测试卡恢复 3 号档。原生版完成读档校验和标题
读档任务退出，重载原角色模型及动作，在第 1634 个场景帧呈现 Gene、R100 房间和 HUD。
运行到 tick 1826，共呈现 1501 个场景帧并提交 1200273 个世界绘制包。渲染器、GPU、后处理
及窗口错误均为 0。下一处原版敌人 Move 是 `0x002147D0`。

同一可执行文件的新游戏复跑再次呈现原版出拳及 3 点伤害，运行到 tick 1810 和 1669 个
场景呈现。下一处原版场景碰撞反应是 `0x0020B394`。冻结源码编译 370 个 C 模块，
通过 351 项原生检查。

## First presented player hit in R100

A hidden, muted Windows run uses synthetic gamepad input to approach an
original enemy and start Gene's Square attack. The original hit applies 3
damage to the selected enemy. Frame 1,637 records HP 74 and frame 1,638
records HP 71 and the matching Take event. Both frames reach the window
after the room fade. The run continues to tick 1,784 and scene frame 1,643,
with 1,320,495 world packets and zero renderer, GPU, postprocess or window errors.

The next original call is the OM324 actor destructor at `0x00198CE0`.
Continued combat, room transitions and full save/load remain under development.

隐藏、静音的 Windows 实跑通过合成手柄输入接近原版敌人，并让 Gene 按方块出拳。
第 1637 帧记录敌人 HP 为 74，第 1638 帧记录原版受击事件、3 点伤害和 HP 降到 71。
两帧均在房间淡入完成后呈现。运行继续到 1784 ticks 和第 1643 帧，共提交 1320495 个
世界绘制包，渲染器、GPU、后处理和窗口错误均为 0。

下一处原版调用是 OM324 物件析构 `0x00198CE0`。后续战斗、房间切换和完整存读档仍在开发。

## First visible R100 room frame and native player movement

A hidden, muted Windows run reaches tick 1,744 and presents scene frame
1,603. Synthetic gamepad input follows the original mapping into Gene's
run state. The first movement reaches the window submission at frame 1,599,
under the fade. At frame 1,603 the final displayed composite has visible
room geometry, Gene, nearby enemies and HUD. The original fade is still active.
The run submits 1,243,415 world packets with zero renderer, GPU, postprocess
and window errors.

Two enemy actors execute the original scenery-contact reaction. The next
unfinished call is attached enemy footstep playback at `0x002CBF78`.
Continuous gameplay, player combat, room transitions and save/load remain
under development. The screenshots and original assets remain local.

隐藏、静音的 Windows 运行到 tick 1744，并呈现第 1603 个场景帧。合成摇杆输入经原版
映射进入 Gene 的跑步状态。位移从第 1599 帧提交到窗口，第 1603 帧的最终画面开始显示
房间、Gene、附近敌人和 HUD，淡出仍在进行。共提交 1243415 个世界绘制包，渲染器、
GPU、后处理和窗口错误均为 0。

两名敌人执行了原版场景物件碰撞反应。当前停在敌人的脚步声请求 `0x002CBF78`。
连续操作、玩家战斗、房间切换和存读档仍在开发。截图与原始资源留在本地。

## Complete native R100 opening and return to the room

An immutable Windows build plays all ten original EV01 camera cuts from the
patched ISO. The recovered quit path retires the opening camera and event
actors, then returns to the R100 scene loop. The hidden, muted New Game run
reaches 1,735 ticks, presents 1,594 scene frames and submits 1,224,912 world
packets. Renderer, GPU, postprocess and window error counts are zero.
The matching native suite passes 355 C modules and 349 checks, with muted
audio devices, movie decoding, two ADX sample comparisons and both RNG suites.

The late opening includes the original costume meshes, animation, scene
lighting and attached effects. One EL31 stream keeps the original zero TEX0
write and samples the previous complete frame's GS display-copy texel. Its
texture color is modulated by the vertex color while alpha comes from the vertex.
Final composites at scene frames 1232 and 1238 show the original characters
and camera change. The native model test also checks this sampling path
against independent RGBA and depth calculations.

The run reaches R100 Scenario task registration `0x002C3890` after EV01 cleanup.
It used synthetic menu input. Continuous player control, combat, room
transitions and save/load still need verification.

固定 Windows 构建从补丁 ISO 播完 EV01 的十个原版镜头，清理开场相机与角色后回到
R100 场景循环。隐藏、静音的“新游戏”运行达到 1735 ticks，呈现 1594 个场景帧，
累计提交 1224912 个世界绘制包，渲染器、GPU、后处理和窗口错误均为 0。
对应原生回归通过 355 个 C 模块和 349 项检查，包含真实静音音频设备、片头解码、两组
ADX 逐样本对照和两组随机数验证。

开场后段包含原服装网格、动画、场景灯光及附属特效。EL31 的一条网格保留原始零值
TEX0 写入，从上一完整帧的 GS 显示拷贝取样。纹理 RGB 受顶点颜色调制，透明度
取自顶点。已检查场景帧 1232、1238 的最终画面，可见原角色和镜头变化。原生模型测试
用独立的 RGBA 和深度计算核对了该采样路径。

运行完成 EV01 清理后到达 R100 的 Scenario 任务注册 `0x002C3890`。菜单使用合成
输入，连续操作、战斗、房间切换和存读档仍待验证。

## Native opening through the first camera cut

The immutable source snapshot passes **316 C modules and 340 checks**, including
muted real audio devices, movie decoding and both independent ADX sample
comparisons. Both RNG suites pass against the same native DLL with 20,544 and
20,534 exact comparisons.

The matching native Windows New Game run reaches 632 ticks, completes the
first EV01 camera cut and enters cut1/frame0. It presents 512 scene frames,
including earlier title and room startup frames, and submits 134,036 world
packets. Renderer, GPU, postprocess and window errors are zero. Final composites
at scene408 and scene457 were visually inspected. The original fade reveals the
street and sky, and the effect moves between the captured frames.

Original model-backed effects use the room's existing model/registry owners,
nested emitters and ordered sound/model cleanup. Framebuffer warp validates
the texels consumed by covered, source-dependent fragments. The current camera
reaches actor drawing and attached lighting. Shadow alignment uses its
independent original Up vector.

The next original frontier is Event actor Move `0x002B6930`, state4 in the
second camera cut. Counters through that stop are player1, child141 and
object9654, with 14 active emitters and 102 particles. Physical gameplay input,
later Event cuts, combat, progression, transitions and save/load remain active
work. Captures and resource buffers stay in private local evidence.

固定源码副本通过 **316 个 C 模块和 340 项检查**，包含真实静音音频设备、片头解码及两组
独立 ADX 逐样本对照。同一原生 DLL 的两组随机数验证分别完成 20544 和 20534 次精确比较。

匹配的 Windows 原生“新游戏”运行达到 632 ticks，完成 EV01 第一个镜头，进入第二个
镜头第 0 帧。共提交 512 个场景帧，包含之前的标题和房间启动帧，累计 134036 个世界包。
渲染器、GPU、后处理及窗口错误均为 0。已检查场景帧 408 和 457 的最终合成画面，原版
淡入揭示街道与天空，两次捕获之间可见特效位移。

模型特效共用原房间的模型和注册表，保留嵌套发射器及声音、模型的有序销毁。
画面扭曲按实际覆盖且依赖纹理的像素验证采样，当前活动相机接入绘制和光照，
阴影仍读取其独立 Up 向量。

下一原调用为第二个镜头中的剧情角色更新 `0x002B6930`，状态 4。截至该处累计完成
玩家更新 1 次、附属模型更新 141 次和物件更新 9654 次，活动发射器 14 个、粒子 102 个。
后续镜头、物理输入、战斗、完整流程、场景切换和存读档仍待完成，截图与游戏资源保持本地私有。

## Previous native R100 world layer beneath the opening fade

An immutable source copy passes **288 C modules / 318 checks**, with muted real
audio, movie decoding, full independent ADX comparison and both same-DLL RNG
checks enabled. A matching actual New Game run reaches 490 ticks / R100 Main4.
The first original scene update/render cycle completes and submits 1,858 native
world geometry packets, with renderer/GPU/window errors all zero.

The world surface before UI was read back and visually inspected: Gene, R100's
street/buildings, sky, textures and shadows are present (228,967 nonblack pixels,
31,463 RGB colors at 512x448). The final composed frame is opaque black because
the original opening Fade is visible with RGBA `FF000000`; no visible interactive
window or continuous gameplay is claimed. The prior 369 title UI frames plus
this one R100 cycle give 370 presented scene frames. Image/resource buffers stay
private; public source contains no original game assets.

The next tick stops at original OM304 state1 `0x00188190`, SceneStage13. Counters
include that partial tick: player1 / child2 / OM81, four ambient emitters and
zero ESP slots. The first cycle included all69 cold objects and four runtime
3EB tutorial objects. Native walk/run animation and root displacement, original
ESP update/dispatch/lifetime, unlit material constants, signed-W clipping and
full packed16 XY raster coverage now have focused tests. Physical analog input,
positive effect postpasses, continuing actor/combat states and progression remain
active work; independent numerical profiles are not PS2 hardware traces.

固定源码副本通过 **288 个 C 模块／318 项检查**，含真实静音设备、片头解码、完整 ADX
独立对照和同 DLL 两组随机数验证。匹配运行进入 R100，完成一次原场景更新／绘制，
提交 1858 个世界包；渲染、GPU、窗口错误均为 0。合成前世界层已实际读回并检查，角色、
首关街道建筑、天空、纹理和阴影均已出现。最终帧仍被原开场 `FF000000` 淡入遮罩覆盖。

运行共 490 ticks，370 场景帧包括 369 个标题帧和一个 R100 周期；下一 tick 停在原物件
304 状态1 `0x00188190`。累计玩家／子模型／物件更新为 1／2／81，包含第二 tick 的部分
进度。尚不可玩，未验收连续动画、物理输入、战斗、切换和存读档；图像和游戏数据仍保留
本地私有，不进入公共 Git。原行走／跑步根位移、粒子生命周期、共享材质参数、裁剪和
完整 16 位坐标覆盖已有分项验证，不将这些数值参考称为 PS2 实机逐位证明。

## Previous milestone: SceneStage22 model-hide/reset frontier

An immutable source snapshot passes 270 C modules / 295 checks with hardware-muted
audio, movie decoding and the full independent ADX comparison enabled. The hidden,
muted synthetic New Game run completes 489 ticks and reaches R100/Main state4.
It completes one original player update, one child update and 73 original object
updates: all 69 cold OMS objects and four runtime tutorial objects from 3EB.
The first original FX call 0x30F228 completes and creates four ambient emitters
(bank3; no active ESP slots). Two RNG routines in the same DLL pass 20544 exact
state/float-bit comparisons across 10272 seeds against a restricted model of
the actual instructions.

The next actual frontier is 0x00308BC0 inside model-hide/reset call 0x00307B28,
SceneStage22. The run submits zero world packets. No world scene frame, playable
scene, continuous animation or combat has been verified. Hardware/device and
decoder checks are enabled; exit code 4 remains a normal unfinished frontier.

固定源码快照通过270个C模块／295项检查，启用硬件静音音频、片头解码和完整独立ADX对照。
隐藏、静音的合成“新游戏”运行完成489个tick，进入R100／Main state4。实际完成玩家更新1次、
子模型更新1次和原物件更新73次：包括全部69个冷启动OMS物件及来自3EB的4个运行时教程物件。
首个原始FX调用0x30F228完成，并创建4个环境发射器（bank3；ESP 活动槽为 0）。同一DLL中的两个
RNG函数对10272个种子完成20544次状态／浮点位精确比较，依据实际指令的受限模型验证。

下一实际边界为SceneStage22中模型隐藏／重置调用0x00307B28内的0x00308BC0。运行提交的世界
绘制包为0；尚未验证世界场景首帧、可玩场景、连续动画或战斗。已启用硬件／设备和解码器检查；
退出码4仍表示正常停在未完成边界。

## Previous milestone 67: complete first player and child updates

The immutable source copy passes 254 C modules / 273 checks, including real
muted audio, movie decoding and the full independent ADX comparison. Its
matching Windows run completes one original player108AB0 update and one
child100530 update. The actual counters show all child force, angle, distance
and finish stages completed. The next unresolved descriptor is the first
door's Move method `0x00186928` (handle159).

This connects the original motion/blend, interaction, solid/wall/floor collision,
ground pose, hierarchy/IK, opacity, position history and Save/Heat updates.
The child uses the same parent parts and persistent spring/template owners.
A real one-ULP acos input exposed an EE COP1 SQRT difference; its explicit
numeric reference follows the original operation, with no input clamp or
PS2 hardware-equivalence claim.

The run still submits zero world packets and stops during MoveObj. It is not
a complete scene frame, continuous animation, combat or a human playtest.
Focused attached-sound tests additionally verify real muted random-pitch ADPCM
voices; complete in-game audio remains unfinished.

固定源码副本通过 254 个 C 模块／273 项检查，含真实静音音频、片头解码和完整 ADX
独立对照。匹配的 Windows 程序实际完成玩家与附属子模型各一次完整更新，弹性骨骼
各阶段均完成；下一处为首扇门 Move `0x00186928`。世界绘制包仍为 0，尚无完整场景
首帧、连续动画或玩法验收。数值参考遵循原 CPU 运算语义，不以夹住输入掩盖差异，
也不声称 PS2 实机位精确。原声音、战斗、存读档和完整流程仍需继续恢复。

## Previous milestone: player first-update motion

An immutable source copy passes 221 C modules / 237 checks, including muted
hardware audio, the movie codec and the full independent ADX comparison. Its
matching Windows QA executes 489 updates: 6 MC, 90 movie and 369 title/menu
frames. Main state4 now completes original NearCheck, world lighting and
positive HitArea sorting, actor filtering, line-of-sight and hit publication.
It then enters the actual player Move method, selects idle motion, binds the
original 10-frame blend and samples its first pose. The next unresolved call
is idle input/interaction `0x00123938`.

Independent motion tests exercise 12 ticks and 32 original PL00 parts, actual
alternate-idle SEQ timing, old-sequence release and late-error resumption. This
is not continuous in-game animation or a human playtest. World packets remain
zero, so the original 3D scene, complete player frame and gameplay are pending.
Enemy initialization and effects work are being verified separately and are
not included in this integrated execution claim.

Self-built evidence tools check instruction bytes against the user's actual
ELF, classify direct calls/imports and summarize static memory operands. The
Ghidra exporter now records instruction limits and truncation explicitly.
See [tool documentation](EVIDENCE_TOOLS.md); raw reports stay private.

固定源码副本通过 221 个 C 模块／237 项检查，含真实静音音频设备、片头解码和完整
独立 ADX 对照。匹配的 Windows 验收完成 489 次更新：6 帧存储卡、90 帧片头和
369 帧标题／菜单。Main state4 已完成 NearCheck、光照和正碰撞排序／对象筛选／
视线／命中发布，真正进入玩家 Move，执行原待机选择、10 帧过渡绑定及首次姿态采样。
下一调用为输入／交互 `0x00123938`。世界绘制包仍为 0，没有完整首帧或可玩场景。
独立动作测试覆盖 32 个部件、12 次更新和失败续接，不能替代整合动画验收。自制工具
核对实际指令、调用／导入及访存操作；大函数截断会明确标出，原始报告不公开。

## Previous milestone: R100 Init and first update entry

An immutable source copy passes 212 C modules / 228 checks with muted hardware,
codec and the full independent ADX comparison. Its matching Windows QA completes
489 updates: 6 MC, 90 movie and 369 title/menu frames. Original R100 Init now
finishes, including all69 OMS objects and four real runtime tutorial objects;
Main state4 enters its first update and stops at NearCheck ray `0x00131E10`.
Roof/intro Scenario tasks are registered but have not been visited yet.

The native input sample now supplies original port0 readiness from the keyboard
adapter; losing focus releases keys without unplugging it. Collision dispatch,
dynamic boxes and shared static/dynamic ray queries pass real-resource tests.
Original room music selection independently reaches real muted PCM consumption;
it has not yet started in this bounded Windows run. No world packet is submitted,
so no original 3D frame or gameplay is claimed. Later updates, Event/actor logic,
continuous animation, combat and save/load remain unfinished.

固定源码副本通过212个C模块／228项检查；匹配的Windows程序完成489次更新，显示
6帧存储卡、90帧片头和369帧标题／菜单。R100初始化及69个OMS／4个教程物件完成，
Main state4首次进入更新，停在NearCheck射线入口0x00131E10；Scenario任务已注册，
尚未遍历。键盘适配器写入原输入连接状态；失焦与断开分开处理。碰撞／混合射线和
首关BGM正分支已分别测试，不能当成整合画面或玩法验收。世界绘制包仍为0。

## Previous milestone: original player and first-room objects

An immutable source copy passes 196 C modules / 207 checks with muted hardware,
codec and full independent ADX verification. Matching hidden Windows QA executes
the original Title/New Game transition into R100/Main3: 487 updates, 6 MC,
90 movie and 368 title/menu frames. All 69 original OMS object initializers
complete after 156 static constructors, full player/child construction, initial
motion sampling, terrain IK, matrix recovery, spawn and camera binding.

The next actual call is the R100 roof task at `r100.rel+0x2F30`. No world packet
is submitted, so there is still no original 3D game frame or playable game.
Separate palette/geometry tests transform 9573 original vertices with canonical
part matrices; this does not establish lighting, clipping or a rendered frame.
Collision dispatch and unequal cylinder argument regressions now inspect the
actual ELF call table. Positive queries outside the recovered domain stop.
Numerical reference profiles and modeled checks are not PS2 hardware captures.

固定源码副本通过 196 个 C 模块／207 项检查；匹配的隐藏静音 Windows 程序完成
487 次更新，显示 6 帧存储卡、90 帧片头和 368 帧标题／菜单。实际运行完成玩家及
子模型、初始动作采样、地面 IK、矩阵回写、出生／摄像机绑定，以及 156 个静态对象
和全部 69 个 OMS 物件初始化。下一边界为屋顶任务 `r100.rel+0x2F30`。
世界绘制包仍为 0，尚无首关 3D 画面；独立骨骼矩阵／几何检查不能代替实际绘制。

## Previous milestone: integrated original title scene

An immutable source copy passes 105 C modules and 111 checks, including muted
device/codec execution and the full independent ADX comparison. Its matching
Windows QA binary completes 600 updates: 6 MC frames, all90 Clover frames,
then481 original title scene frames with no unresolved consumer. A captured
frame shows GOD HAND / PRESS START. Original timers advance the logos; title
input is not injected in this bounded acceptance run.

The actual OT traversal now orders logical banks, depth buckets and LIFO packet
groups. Text stays one packet group; each numeric digit is separate. Completed
pixels are presented on the following update, before borrowed resources expire.
Canonical cameras, display state, object registries, scene updates and HUD
consumers share their owners. Explicit zero-memory and binary32 reference
policies remain separate from PS2 hardware validation.

A separate New Game QA reaches the difficulty menu and then the actual bank2
sound boundary2CB8A0. State5/SystemMenu/TitleEnd and sound release are covered by
bounded original-owner tests; the integrated first world scene is still pending.
No original combat, player animation, full progression or save/load is claimed.

当前固定源码副本通过105个C模块/111项检查；匹配的Windows程序完成600次更新，
显示6帧存储卡界面、90帧片头和481帧原标题场景。已捕获PRESS START画面。
另一“新游戏”验证到达难度菜单后停在真实bank2音效入口；第一关和可玩玩法仍未完成。
以下条目保留历史节点，最新范围以本节及progress.json为准。

## Milestone 1: original input and native foundation

- Indexed the user's patched NTSC-U disc: 1445 resources and 81 dynamic modules.
- Implemented native C read-only original-disc access and verified an actual
  room resource byte-for-byte without bulk extraction.
- Recovered two RNG functions, one boot dependency slice and six small room
  functions into native C, with explicit original addresses and provenance.
- Built native x86-64 code and passed RNG, boot, resource, room and C++ linkage
  checks. No missing platform operation silently returns success.
- Compared two RNG routines against a restricted model executing instructions
  read from the actual ELF: 10272 seeds, 20544 exact state/float-bit comparisons.
  This is modeled evidence, not PS2 hardware validation.

## Work still required

Complete the original frame loop, title/scene and model/animation consumers,
remaining audio/input/save services, then verify original combat, transitions
and progression. Native startup, MC and Movie0 integration are described below.
The current room slice covers its small allocation/init/move behavior only; it
does not render or enter a playable room.

The upstream project's PS2 matching percentage is not our Windows-port
completion percentage. The patched boot ELF differs from its retail target;
all claims must remain tied to the actual input and verification scope.

## Milestone 2: lifecycle and real host memory

- Recovered r207's three remaining lifecycle routines from its actual zero-count
  constructor table and empty destructor list. This exact module has no observed
  nonempty constructor/destructor sequence; no general CRT was invented.
- Added real native aligned allocation, bounded payload capacity, ownership and
  explicit error handling. The room initialization test allocates/writes/releases
  actual host memory. This does not claim complete original cHeap behavior.
- Recovered three task-manager functions: slot selection, attribute inheritance
  and filtered visitation, including changes to flags/count during traversal.
  This milestone did not yet connect thread step/start/wait/kill.
- Build evidence fingerprints the actual inputs; progress export rejects changed
  sources or a stale DLL differential result instead of reusing old pass labels.

## Milestone 3: native startup-task integration

- Real Windows fibers implement the observed serial task start/sleep/exit
  handshake. Unsupported EE kernel behavior remains explicit.
- DVD jobs read actual sectors into owned aligned allocations, including real
  sector padding. Priority selection, queue-pressure drain and the unconditional
  extra WaitFrame sleep are recovered from the actual executable.
- Recovered task `0x0030D358` runs with real native sound-bank, DVD, font, menu
  and ID adapters. An integration run reaches the next task request after five
  scheduler ticks; these are not timed video frames.
- Native sound startup owns program/sample tables and indexes actual external
  AFS directories, including BGM0. It does not synthesize RPC acknowledgments,
  execute voices or produce audio output.
- Default next entry `0x002AE8A8` is the memory-card boot task. A literal `jal 0`
  inside it is resolved by SNR2 relocation to `mcport.rel`'s `Main__7cMcBoot`;
  treating it as a no-op would skip original behavior. The host reports an
  explicit frontier instead of installing an empty task.
- Original texture RGBA passes native decoding and RTX offscreen upload/readback.
  GPU asset QA does not establish game rendering or gameplay.
- Restricted original font packets are decoded through CT32 upload storage to
  PSMT4 sampling. The original code's odd/even glyph CLUT selection is preserved;
  native glyph RGBA is distinct from final screen-space drawing and blending.
- The later `0x0030D5C8` loader separately passes real core, room-jump,41-object
  table and all eight player-costume read checks. The table is independently
  checked against the actual ELF; this is not full GameMain execution.
- Native memory-card queries and recovered slot/file checks run against an
  actual host profile directory. Localized/fallback idsavecore loads bind slot11
  without replacing slot1. Full MC Main still needs IDBase/UI and save semantics.

## Earlier local continuation: original GameMain case0

- The alternate original flags branch now registers the recovered GameMain and
  asset-loader tasks. Real no-draw gates and all eight case0 subsystem
  initializers run; the loader finishes concurrently before the host reports
  the next unavailable state1 handler (`0x002A5BB8`). This is a developer branch
  execution slice, not the default complete game startup.
- Original CoreSave system/game initialization preserves fields according to
  the actual writes. Enemy/object pool bounds, linked-head reset and the
  separate624-word random generator are explicitly recovered and tested.
- Actual67-entry MC IDBase initialization, typed pools, parent linking/reset
  and the packed-message consumer pass original-resource tests separately.

原 GameMain 的case0与并行资源任务已能在开发宿主中真实执行，接下来停在case1。
默认存档启动路径已连接真实模块核验、图标分配、67个ID初始化、packed环境和
初始DispInit，停在模块Control循环；仍未到达可玩场景。原T32存档素材已解码，
ID的实际移动、颜色动画、绘制顺序和受限纹理quad也已分别验证；完整窗口绘制待接。

The development host begins at the recovered startup task. Earlier boot
initialization, the complete main loop and the actual next tasks are still
required. No playable scene or controls have been demonstrated.

## Milestone 4: native Windows MC, movie and Main continuation

- A single hidden native HWND/D3D11 run presents six actual MC interface frames,
  executes normal MC cleanup and same-slot tail restart, then presents all90
  Clover movie frames. Real videoEOF and audioDONE complete before continuation.
- The run uses an explicit synthetic digital input fixture, through the original
  ELF table. It is not a human input/playability claim. QA output is muted while
  the decoder/device queues and completion tracking actually run.
- Original GameMain setup advances through states0/1/2 into3, using canonical
  save, camera, HUD, room and task owners. Actual r020 title-room resources and
  RHT-based pools load. The precise next frontier is main ELF002BE708, native
  room overlay binding; no title scene or combat is claimed.
- The MC GPU consumers preserve actual draw order, original palette, font
  nearest sampling and separate T32/TIM2 four-bit bilinear sprite math. Real
  cursor and window pixels match independent CPU equations. Reusing the common
  renderer preserves the initial MC frame's byte hash.
- All55 C modules and56 checks pass with hardware/codec opt-ins enabled. An
  independent clean staged-source foundation snapshot also passes55/56. Separate
  Windows targets rebuild every C module and fingerprint all consumed sources.
- The normal native target accepts actual window/device input. Synthetic keys
  exist only in the separately compiled startup-QA target. Unknown consumers
  remain explicit frontiers. Earlier boot and complete game frame timing remain
  unverified; the executable is not yet a playable game.

## 中文进展摘要

同一个 Windows 原生程序已串联原存储卡可见界面、正常退出、Clover 片头和主游戏初始化，
真实加载标题房间资源后停在002BE708模块绑定。隐藏静音验收显示6帧存档界面和90帧片头，
使用明确标注的合成按键；视频EOF及音频设备DONE均实际完成。55个C模块通过56项检查。
完整主循环、可玩场景、模型/动画、战斗和存档读写仍未完成，不以此宣称玩家实测或移植完成。
当前测试和限制以 [progress.json](progress.json) 为准。

## Local continuation: original scene setup and outer-frame leaves

The same hidden native startup now reaches002D6F40. It actually loads and
version-binds R020's module identity, performs the preceding Scenario leaf,
commits the real room SE bank, initializes collision records, effect resources
and room filter presets. R020's exported Init/Move have not been claimed as
executed.64 C modules/66 checks pass; Windows QA still presents6 MC/90 movie
frames before the precise new frontier.

Known outer-loop order now places Movie update before tasks and Fade after
tasks, ahead of DVD/audio. Recovered frame policy preserves the separate FPS
and mode bytes. The observed boot seed and two-word MT advance share the actual
GameInit MT owner. The finite conversions pass20534 comparisons across10267
supplied MT words against their actual restricted instructions. This reference
is independent of host rounding and remains distinct from hardware/full-boot
validation. New Fade GPU primitives are available; unknown frame surfaces remain
explicitly unsupported.


## Local continuation: scene setup, native ADX and direct title UI

84 C modules pass85 checks with device/codec/reference opt-ins. Hidden Windows
startup reaches002BF8F8 after native world-light defaults, model-packet reset,
Cockpit room reset, collision/table setup and the original title-player
constructor. Earlier boot, the rest of case3 and complete state4 remain pending.

Native encrypted ADX decoding matches all6369200 stereo sampleframes against an
independent vgmstream output. Bounded native streaming passes real muted device
consumption, pause, loop, seek, stop and EOF checks. Original BGM request Set,
Start and Stop execute supported actual paths; unsupported paths remain explicit.
The original logo SFX uses its real bank/program/sample and device lifetime.

A separate direct R020 Init/Move GPU fixture renders the original warning
screen using its actual IDD. It has an explicit512x448 black reference clear;
it does not bypass missing scene operations in production, establish final OT
order, or demonstrate a playable game. The earlier MC first-frame hash remains
unchanged after reusing the renderer and adding the rotated polygon backend.

本地检查点：84个C模块通过85项检查，隐藏Windows主流程到达002BF8F8。原加密ADX
逐样本验证和真实静音设备队列通过；原标题Init/Move的独立GPU验证已画出警告首屏。
完整case3/state4、最终绘制顺序、摄像机、模型/动画、玩法和存档仍需继续，不宣称可玩。
