# Fighting Vipers: progress

Disc: Fighting Vipers (USA), MK-81041 V2.000 (17 tracks: data + CD audio).

## The training mode, run-ahead, the dummy at random (2026-10-14)

Features taken over from the CPS-2 ports (Vampire Savior's), on satrecomp at
e11a3a4 (its branch `cps2-features`).

**The game's training mode is mode 5**, not 7. Chosen from the main menu (TRAINING MODE, two below ARCADE
MODE), the byte at 0x060E0002 is 5, and its fight is not state 8 (`kFighting`) but 21 as it begins and then
22 for as long as the fighters fight (`state::kTrainingBegin`, `state::kTraining`), with "PRESS START TO
OPTION MENU" at the top and its own DAMAGE counter. Its gauges take nothing: `TakeDamage` leaves by the
time-up or the no-damage way. Mode 7, which the round's code treats as counting no rounds, was seen
played from the PLAYBACK MODE screen; it is named `kModePlayback` now (and its state 13
`kAfterRoundPlayback`). The training aids had only been tried in arcade fights, where they were made; in
the training mode the dummy did nothing (it waited for state 8) and the damage readout had no hits to show.
Now `Fighting()` in `training.cpp` takes state 22 as a fight too, and `TakeDamage` reports what a hit is
worth to the aids in the training mode however it leaves.

**Training aids only in the training mode.** Every feature of the categories Training, Dummy, Dummy counter, Dummy recovery and Dummy wake-up is
marked a training aid (`kFeatureTraining`), and `Game::inTraining` is the mode being 5: elsewhere they read
as off, while the menu and settings.ini keep the player's choice. Armour is not an aid (it changes the
game, as the cheats do). Checked: `movies/own_auto_7` (arcade) with the readouts and the dummy on gives
2,152 readout changes on the old build and none now; with every feature off the state hashes are the
same as before for 6,000 frames.

**Run-ahead** (the toolkit's; satrecomp/docs/FEATURES.md): the game's sources skip, in frames run ahead,
what is not part of their save state: the table of moves (`Tell`, its counts and file) and the dummy's
recording; the move log is written when the frames come again. State hashes of `own_auto_7` the same with
it off, on 1 and on 2.

**The dummy at random** (`training.cpp`): "At random" for Dummy recovery (chosen as it goes down) and Dummy
wake-up (as it gets up; the recording among the choices once there is one); Dummy struggles in throws is
Off, Always or At random (as each throw begins); "Dummy answers now and then": counters and wake-ups one
time in two (one chance per attack answered). The chances come from the training state's own row of
numbers (`Chance`), so the reset point and save states give the same ones again. Seen in the training
mode with hand-made movies: wake-ups after 20 knockdowns by Grace's 4K vary (2K, K, GP, P); counters 21
of 21 without "now and then", 17 of 27 with it; throws escaped 6 of 6 always, 2 of 3 at random. Struck
out: a "mash out of stun" option as the CPS-2 ports have, since this game has no stun (below).

**Move log**: `frame_data.csv`, a line an attack: player, character, the input as a button was last
pressed for it, move, kind, level, start, active, recovery, result, advantage, damage, armour damage.
Written once the result is known, or a second after the attack was over and the other is not still
reacting to it, or when another attack takes its place. The frame data readout's throw line says
ESCAPED when the throw took nothing off the gauge, also when it took armour instead (as seen in the
training mode, Grace on Grace: 160 armour); the log says `throw` then.

**Widescreen in the menu** (the toolkit's): a run switched at frame 1200 gives at frame 1300 the picture,
byte for byte, of one started in widescreen. The title screen in widescreen (as before) shows a band with
the logo moved across: not looked into.

Not done by a person: run-ahead felt in a window (it was measured and compared without one), the window
taking widescreen's shape, the new dummy choices played by hand, the launcher's new rows used.

## Throws, the moves' data, and no stun (2026-10-12)

Found for the training aids of that day (`src/training.cpp`: throw range, the throw's frame data, the table
of moves, the dummy's counters and wake-ups); names in `src/fv.h`, the routines rewritten in `src/throws.cpp`.

**Requests.** What a fighter does next is a request, a long in its record (+0x3C): a kind in the first
byte, a move in the last word. The reading of commands is a small script machine (the loop at 0x06015C12,
an opcode a step from the table at 0x0607A898, the scripts in the players' command tables); its opcode 5
(0x06015CE0) leaves a request, and others test the fighter's state, the input's history, and the bits of
+0x578 (opcode 0x36) and the distance at +0x4D4 (0x1A, 0x1B) that 0x06010C40 works out. 0x0600CB8E
(`PendingRequest`) lets a request the game forced (+0x64) or put off (+0x68, until the move's frame is the
word at +0x502) go before the commands'; 0x0600CC02 takes the request up as the action (+0x6C) by the first
routine of its kind in the table at 0x06011608, and the second routine there runs each frame, by the step
at +0x5A.

**Throws.** Guard and punch beside a standing fighter is the request 0x10000015 (kind 0x10, move 0x15).
0x0600C974 (`AnswerThrows`) answers a throw's request by giving the other fighter the request to be thrown
(0x11, with the move from the throw's record: 0x16), for four kinds of throw (0x27, 0x25, 0x10, 0x2B; only
0x10 was seen done); if both ask for the same throw on the same frame the round's clock decides (an even
count: player 1's). A fighter thrown while it could not act takes a quarter more. The thrower's state has
bit 18, the thrown fighter's bit 19. The throw happens on the frame after the buttons are read: there is
no start-up, and no whiff either, for out of reach the same buttons are an ordinary attack (move 0x159,
for Grace and for Bahn: 50 frames, hits on the 15th).

Whether a throw can be asked for is the bits of +0x578 (`reach` in `fv.h`), which 0x06010C40 sets for each
fighter against the other every frame: the thrower not in the air; the other not in the air, not hit, not
down, and standing, walking, guarding or recovering (so not starting an attack up, nor hitting); the
thrower facing the other within 30 degrees; then, with the other facing the thrower within 30 degrees,
1.7 m between them (1.6 m if the other crouches), or with the other's back turned 1.4 m. The distance is
between the fighters' places (+0xDC, +0xE4: where the hips are), kept at +0x4D4. For 14 frames after any
hit or guard (the count at 0x060E0022) nobody can be thrown. Within a metre of a wall other bits are set
(0x100, 0x200) and the script asks for other throws; a fighter lying down has bits of its own (0x40, 0x80:
within 1.8 m). Tried: the bit comes on at 1.70 m walking in; a crouching dummy gives the crouching bit at
1.6 m; nothing while the dummy jumps or attacks, or for the 14 frames after a guarded punch.

**The way out of a throw** is to struggle. While a fighter is thrown its presses are counted (+0x6C8, one a
frame at most), and the move of being thrown names a second move to go on to if the count is over a number
(+0x690, +0x692; 0x0601245E). Thrown by guard and punch: move 0x16, 52 frames; with 19 presses or more by
its end the fighter goes on to 0x1AA, breaks free on its feet and takes nothing (seen: the gauge stays at
250, and it can act 67 frames after the grab, 7 after the thrower); otherwise 0x1AB, the damage (40) on the
75th frame of the throw, and it lies down (it can act 112 frames after the thrower). There is no other
escape in the code that answers a throw or in the routine of being thrown (0x0600F0C4).

**The moves' data.** A table of pointers by move number (0x0606D304), each to eight bytes and then records
by type (0x06012428, `MoveRecord`, finds one; lengths by type at 0x0606C6EE). Type 2 is the frames: the
frame it hits from, the frame after the last it hits on, the frame its fighter is free from. Type 4 is an
attack: the body parts that attack, a power, and a level. The damage is worked out from the power: three
quarters of it off the gauge where the part hit is armoured and one and a half times where it is not (seen
on Bahn with his armour and with it taken off: the punch 9 and 18, down and kick 17 and 34; the kick 22 both
times, so which part is hit counts as well), the power itself onto the armour's damage and half as much
again if guarded. Armour changes what a hit leaves too: Grace's punch is 0 on hit against armour and +5
against none. 0x06011BF4 starts a move and copies these into the fighter's record (+0x63C on).
The level decides what a guard does (0x06024DAE on, the routine that works out a hit): 0 misses a crouching
fighter and is guarded standing (high); 1 is guarded standing and hits a crouching guard for three quarters
(middle); 2 hits a standing guard and is guarded crouching (low); 3 as 1 but in full; 4 is the only kind to
hit a fighter lying down. Tried with Grace against Bahn: the punch (0x25F: high, power 12) and the kick
(0x2F6: high, 30) pass over a crouching dummy and are guarded standing; down and kick (0x201: low, 23) hits
the standing guard for 17 and is guarded crouching, 10 frames behind.

**What has and has not been tried.** Every one of these aids was run against the game with scripted input
and no window, and the numbers above are from those runs. The window's own parts (pause, frame step, speed,
the keys, the GPU's readout) were run in a hidden window with scripted keys
([satrecomp/docs/FEATURES.md](../../satrecomp/docs/FEATURES.md), "Trying the window's own parts"), which
found and fixed two faults. Nobody has played any of it by hand; throws from behind, at a wall and on a
fighter lying down are read from the code and were not done; the characters other than Grace and Bahn were
not tried, and their numbers are not known.

**No stun.** The game keeps nothing of the kind. Ten punches in a row at a standing dummy, and the only
things in the dummy's record that add up from hit to hit are the gauge's loss and the armour's damage (one
more that seemed to, at +0x734, is the square of a distance: 0x06021C22); nothing that falls back with
time, and no state came of it. (The attacker's record counts its hits at +0x54C, +0x1250 and +0x1E00,
which only go up.) So there is no stun readout and no stun setting for the dummy.

## Other systems and release preparation (2026-10-12)

Built for Linux x86_64 and Windows x64 as well (`tools/release/`, docs/BUILDING.md), and macOS as one app
for Apple silicon and Intel. What was checked, all without a window: movies/own_auto_7 replays for 6,000
frames to the same per-frame hashes (state, picture, sound) on macOS arm64, macOS x86_64 under Rosetta,
Linux in a container, Windows under Wine, and (3,000 frames from start-up) on a Steam Deck with its own
copy of the disc. The GPU renderer on the Deck's Vulkan driver (SDL's `offscreen` video driver; `dummy`
gives SDL_GPU no backend on Linux) against the Mac's on Metal: 15 pictures of a fight in widescreen with
the training displays, 0.19% of pixels differ. Speed on the Deck, one thread, nothing read back
(`SATRECOMP_GPU_NO_READ=1`): 109 frames a second in software, 95 on Vulkan at 1280 x 800, 80 in widescreen,
71 with every training display on. Not done: seen in a window or heard on Linux or Windows, a gamepad
there, a Windows PC, an Intel Mac.

No emulator's code is in the tree: the VDP1's counters are `video/counters.h` (in place of a copy of
Ymir's file; compared with it on 4.7 thousand million cases), Ymir is fetched by `make setup`, and the
patch that makes it the reference machine is kept outside the repository (`SATRECOMP_REFERENCE`).

## Features on the readable code (2026-10-11)

Quick rounds, damage, input and armour readouts, gauges that refill, armour kept on or taken off, and a training dummy (`src/round.cpp` `QuickWait`,
`src/training.cpp`; [satrecomp/docs/FEATURES.md](../../satrecomp/docs/FEATURES.md)). All are switched from the
launcher's Features page or the in-game menu, and are a few lines in or beside the rewritten routines.
Seen in headless screenshots and memory traces; not played by hand, and the readout's panels in the GPU
renderer's window (as opposed to its screenshots) have not been seen at all.

The dummy is a few lines in `PlayerInput` (`src/fightinput.cpp`), where a player's input for the frame is
taken: a fighter's input is the byte at +0x1224 of its record (punch 1, kick 2, guard 4, down 0x10, up
0x20, right 0x40, left 0x80), from the pad where a person plays and from the tasks before the input task
(0x0601E680, 0x0601E784) where the game plays. The dummy was tried against the game's own play only
(arcade mode), not with two pads.

Hits, as found for the hit view (`src/fv.h`, `hit`; not rewritten): the hit task (0x06004BB8) runs each frame
before the fighters'. It has each body as 22 balls: 0x06004B3A works out their centres (0x060E01D8 for
player 1, 0x060E03E8 for player 2; three longs each) from the table at 0x06009020, 0x06004A44 their sizes
(0x060092D0, 0x06009328: the character's own from 0x06009380, doubled for the parts in the fighter's
attacking mask at 0x060672A4 / 0x060672A8). 0x06004F14 picks which test applies and fills the tables of
what touches what (0x060E00D0, 0x060E0128). 0x06004D68 works out the parts that cannot be hit (+0x560 of
the record), and 0x06004E12 decides the hit: the move's attacking parts (table at 0x060094E4 by the byte at
+0x65C) against the other's parts, once the move's frame has reached the word at +0x644; the parts hit go
to +0x564 and +0x568, and if both hit each other the moves' levels (+0x65E) decide. 0x060051A4 and
0x06010C40 are the fighters pushing each other apart by the distance between their hips.

The view, as found for the hit overlay (`src/fv.h`, `view`): where the fighters are about to be drawn
(0x06017A0E in the drawing task 0x06017B88) the 3D library's current matrix (its address at 28 from GBR;
the stack of matrices is at 0x06052074) takes the world to the view, in units of 1/300 of a metre
(each body part is drawn with its origin at 300 times the hit task's ball there, for every character seen;
the long at 0x06060164, once taken for a scale, is not one), with z the other way round from the hit task's. The vertex routine's perspective is the
table at 0x0608064C: about 682 / z across and 560 / z down, in 704 by 448 dots. 0x06056B0C pushes the
library's matrix, 0x06056B80 pops it, 0x06056AE0 puts a point through it, 0x06059128 draws a part with the
matrix at r4.

A fighter's state, as found for the frame data (`src/fv.h`): the move and its frame are words at +0x50 and
+0x52 of the record; the long at +0x2C has bit 0 while the fighter cannot act, bit 1 before an attack
hits and bit 2 after, bit 8 with an attack until it hits, bit 9 standing free, bit 13 guarding, bit 17
being hit, bit 14 knocked down, bit 16 on the ground (what the buttons do there is with the names in
`fv.h`). Grace's punch: 24 frames, hits on the 9th, 0 on hit and -2 on guard against Bahn; her kick
hits on the 14th, +2 and -1.

Armour, as found for the training aids (names in `src/fv.h`; none of this code is rewritten yet): each
fighter has an upper and a lower armour with a strength of its own (longs at +0x40 and +0x44 of the record:
300 and 100 for Grace, 500 and 200 for Bahn) and the damage each has taken (+0x48, +0x4C), which is kept from
round to round. Hits add to the damage at 0x06025AC4 (the move's armour damage, the byte at +0x65E of the
attacker's record, doubled or half as much again by two of the victim's flags). At 0x06025E20 armour whose
damage has reached its strength is marked as ready (bits 0 and 1 of the byte at +0x5F), and the next hit of
the right kind (0x06025BF6 on) takes it off: bits 2 and 3, the model changed by 0x0600AFFC or 0x0600B044
(0x0600B124 dresses a fighter by the bits), the weight worked out anew (0x06012EA0: body, plus each armour
still on) and the armoured parts of the body (0x06025F14). With both off, bit 4 of the record's first byte.
The display's figures (0x0602FCE8) follow the bits one way only; 0x0602E2C4 draws the display anew.

## Readable code (2026-10-11)

The game's rules for a round, rewritten by hand as readable C++ (`src/fight.cpp`, `src/round.cpp`, `src/fightinput.cpp`; names of
the game's memory in `src/fv.h`), each routine checked against the game's own code call by call
([satrecomp/docs/READABLE.md](../../satrecomp/docs/READABLE.md)). `make check-rewrites`: 2,005,937 calls
compared over the fight movie and some hundred variants of it, none differ; every instruction of the game's
routines was reached except one that cannot be (0x0601D414).

| Address | Name | What it does |
| --- | --- | --- |
| 0x0601C4C4 | FightStep | A frame of the fight: the routine of the state it is in (for the states below) |
| 0x0601C55C | JoinIn | The other player comes in by pressing Start, where that is allowed |
| 0x0601CAA4 | RoundSetUp | State 3: counts the round, sets the clock, starts the fight's tasks, gives out the gauges, has the round announced |
| 0x0601CD3C | RoundCountdown | State 4: the announcement, until its countdown is over |
| 0x0601CE7C, 0x0601CEAA | Begin, BeginWait | States 5, 6: the picture before the fight; "Fight!" |
| 0x0601CEEC | LetGo | State 7: the fighters are let go, the clock runs |
| 0x0601D014 | Fighting | State 8: until somebody is down or time is up |
| 0x0601D0F0 | RoundOver | State 9: who won (also on time, and in the deciding round), rounds counted, the match, team play and versus records, what is announced |
| 0x0601D436 | RoundOverWait | State 10: until that has been shown |
| 0x0602F680 | JudgeTimeUp | On time, whoever has the larger part of their gauge left wins |
| 0x06007E08, 0x06007DEC | Gauge, GaugeSet | A player's gauge, and the one set for them |
| 0x06014C06 | InputTask | The fight's input, once a frame: each player's (`src/fightinput.cpp`) |
| 0x06014C44, 0x06014CC4 | PlayerInput | A player's input for the frame: from the pad's buttons by the button settings, where a person plays |
| 0x06014D44 (to 0x06014FFC) | InputStep | Keeps the history of the input (64 frames: held, pressed, let go), restarts the reading of commands where the fighter's state asks, says which way is forward. The reading of commands itself (0x06014FFC to 0x06015870, about a thousand instructions) is still the game's code, called from here |
| 0x0600FE80 | FightersTask | The fighters' frame: each one's own, then what each does to the other |
| 0x060117D4, 0x060117D0 | TakeDamage | Takes damage off a fighter's gauge; knocks them out at nothing (the entry at 0x060117D0 was never called: not checked) |
| 0x0600C974 | AnswerThrows | Each fighter's request for the frame; a throw asked for by one becomes the other's request to be thrown (`src/throws.cpp`) |
| 0x0600CB8E | PendingRequest | Which request a fighter has this frame: one the game forced or put off before the commands' |
| 0x06012428 | MoveRecord | The record of a type in a move's data |
| 0x06024CFC | RoundClock | Counts the round's time down, has it drawn, says when it is up |
| 0x06024D58 | CountTimeTaken | Counts the time the round has taken (1/100 s) |

The cheats are lines in these routines now, not values written over after each frame.

Learnt on the way (all in `fv.h`): the game's frame is 0x06022640 (pads, the main state's routine from the
table at 0x0605F3A0, then the task list, 0x06004556); a fight is a main state whose routine, FightStep,
runs the fight's own states from the table at 0x0605F1DC; modes 3 (versus, with records of who beat
whom), 4 (teams, the winner staying on with what is left of their gauge) and 7 (no rounds; was taken for training,
see 2026-10-14) are named by what the round's code does for them and were not played; a drawn match is decided by a round of
half the time with one point of gauge each; the gauges set are in low work RAM (0x00200E24), 400 with the
switch at 0x002000A0.

Known and not rewritten: the reading of commands from the input's history (0x06014FFC to 0x06015870, with
the players' command tables at 0x060A4000 and 0x060A5000; it sometimes returns with the stack moved, which
is why its caller keeps the stack pointer); the game's own play (tasks 0x0601E680, 0x0601E784); a fighter's own frame (0x060106D8) and what it does to the other (0x06010C40); the
fighters' moves and the handling of hits, which is one piece of code entered by jumps (0x0600E800 to
0x0600FE80: it works out the damage and calls TakeDamage at 0x0600FCF8 and elsewhere); the states before a
round (0 to 2, which load) and after it (11 on); the clock's drawing (0x0602F094); the task list's caller
(0x06004560; it times each task, so it cannot be compared).

## Features (2026-10-11)

On the game's own machine (satrecomp/docs/FEATURES.md):

- Widescreen (16:9 fights; the gauges and names at the picture's edges).
- An in-game menu (Esc / F1 / Back, alone or with Start / stick click; the Start and Back combination
  has not been tried on a real pad): save states, the features' pages, speed, the reset
  point, "Quit game" (the only way out but closing the window: Esc opens the menu now). It is the menu the
  recomp ports share (`satrecomp/recomp-common/`, a copy of recomp-common; `port/menu.cpp` is a thin adapter).
  Seen in headless screenshots at the software and the GPU picture's sizes; not seen in a window, and Esc
  in a window not tried by hand.
- Save states (F5/F9, slots 1-9).
- Cheats: no damage (either player), one-hit knockouts, infinite time.

What had to be learnt about the game for these: its 3D code does not cull at the screen's sides
(vertex transform 0x0602331C, polygon list builders around 0x060230B4, list sent by indirect DMA); the
fight's whole 2D display is the scroll screen NBG2; the fighters' records are at 0x060627D4 and 0x060646D4
(gauge at +0x54, a word, 250 full), the round's clock at 0x060E005C (1/64 s). Widescreen, the menu and
save states needed no routine of the game rewritten.

## Status (2026-10-11): no emulator in the game

The game now runs on satrecomp's own machine (satrecomp/docs/MACHINE.md). `build/fighting_vipers/fighting_vipers`
has no code of the Ymir emulator in it (no symbol of it, and the default build does not need its sources);
Ymir is only the reference in the development build (`make ref`).

- Own, with the check each passed:
  - SH-2 code: translated C++ (17,306 blocks, 1,961 functions); against Ymir's interpreter, state after
    every frame (`make verify`, reference build).
  - SH-2 interpreter (for code with no translation): against the translated code, state after every frame,
    9,000 frames of two movies with everything interpreted (`make check`). Decode table identical to Ymir's.
  - Video renderer: pixel for pixel against Ymir's, 24,000 frames.
  - Sound chip (registers, timers, interrupts, DMA, DSP, voices): state by state against Ymir's, 635 million
    steps. Sound output byte for byte what the build before made.
  - 68000 (sound CPU): instruction by instruction against Ymir's, 401 million instructions (registers,
    flags, cycles, writes). The sound driver runs in it.
  - Machine (bus, interrupts, timers, DMA, video timing, SMPC, CD commands): no direct comparison possible
    (it does not keep Ymir's time). The first 750 frames are pixel for pixel Ymir's; the commands the game
    gives the SMPC and the CD drive are the same in kind and number; opening movie, title, menus and fights
    run; three random-input runs of 20,000 frames each with nothing unknown asked of it.
  - Disc image reader (cue/bin) and save memory in the console's format (an existing save reads back).
- Speed, headless at the original resolution: about 335 frames a second (the build on Ymir: 286). A full
  build takes about a minute.
- Movies recorded on the reference machine (movies/native_*, idle) still replay but land differently after
  a while; movies/own_* are recorded on this machine.
- Not done, known: the 68000 sound driver is interpreted (not translated); `--video-stats` and the
  cycle-exact SH-2 path exist only in the reference build; nothing here has been played by hand yet on the
  new machine beyond scripted and random input.

## Status (2026-10-10)

(The state before the game had its own machine: what ran on the reference machine, which is now
`make ref`.)

- **No BIOS image is needed.** The port starts the disc itself (satrecomp/runtime/hle_bios.cpp): it
  loads the disc's start-up program and first file, and provides the BIOS's services as C++ routines:
  interrupt entry and exit, the system service table (handlers, SCU interrupt mask, clock change,
  semaphores), slave CPU start-up and the backup memory library (saves: FVIPER_MAST is written and read
  back). Menus, arcade, versus and training fights run this way.
- **Files load natively.** The game's file loader (0x06029B44) is the first routine replaced by
  hand-written C++ (games/fighting_vipers/src/files.cpp): the file comes straight from the disc image
  instead of through the emulated CD drive, so loads take no time (title screen about 4 seconds
  sooner). The intro movie and CD music still go through the CD block.
- **The sound driver is translated too.** The 68000 code the game loads into the sound chip (about
  8,000 instructions, 1,424 blocks) runs as compiled blocks made of the interpreter's handlers with
  constant opcodes: cycle exact, so the sound output is identical sample for sample
  (`make verify KIND=exact` now hashes the sound as well). No 68000 instruction is interpreted in the
  verified movies. This is the first stage (as the SH-2's exact path was); its own plain C++ is not
  written yet.
- **The game's SH-2 code runs as satrecomp's own C++** (`--cpu native`, the default): plain code on the
  registers with direct work RAM access, no interpreter handlers, free running (blocks run to their
  end, nominal cycle counts). About 13,800 blocks, 173,000 instructions (220 KB of the game's code):
  what random-input explorers ran, plus what static discovery found by following branches and calls
  from there (about 2,000 blocks that never ran).
- Blocks joined by branches with fixed targets form native functions (1,784 of them): branches and
  loops inside a function are gotos in one C++ function. The CPU leaves native code at calls, returns
  and computed jumps, and hands over at a backward branch once its time slice is over.
- Verified: `make verify FRAMES=15000 MOVIE=games/fighting_vipers/movies/native_71.txt` (and
  native_74 for 12,000 frames): every block run is native code and the machine state matches the
  interpreter run the same way (`--cpu free`) after every frame.
- The cycle-exact path (`--cpu recomp`, identical to the unmodified interpreter `--cpu pure`) is kept
  as a reference: `make verify KIND=exact MOVIE=games/fighting_vipers/movies/explore_12.txt` (this
  pair runs through a BIOS image in bios/, because the unmodified interpreter has no native BIOS).
- The picture is satrecomp's own (satrecomp/docs/VIDEO.md). In a window the GPU draws the polygons at
  the window's resolution (SDL_GPU, Metal) and puts them together with the VDP2's layers; headless,
  and with `--video software` or the launcher's "High resolution" switch off, the picture is made at
  the original resolution, pixel for pixel what the reference renderer gives (checked over 12,000
  frames each of two movies).
- Speed, fights, one thread: about 300 fps headless in software. Where the time goes there: the VDP2's
  layers and composition about half, the VDP1 rasteriser and the sound generation about a sixth each,
  the SH-2 dispatcher about 5%, the translated game code itself about 3%.
- Launcher (`make launcher`) and app bundle (`make app`). The launcher is the one the recomp ports
  share (`satrecomp/recomp-common/launcher/`; `launcher/src/description.cpp` is what this toolkit
  tells it): built as the old one was, its pictures were byte for byte the old one's (every page,
  five situations, two window heights; taken with no window). Two things then changed on purpose:
  "Full screen" is off unless switched on (it was on unless switched off; a `launcher.ini` that says
  nothing now means a window), and the Controllers page lets each player's keys and gamepad buttons
  be chosen (`port/bindings.h`; `settings.ini`, `[keyboard1]`, `[gamepad1]`, ...). The frontend
  reads them once when the window opens and then reads input from that table, as many lookups a
  frame as the fixed tables took; with nothing chosen the layout is the old one. A gamepad button
  given to the game no longer also opens the menu. Not tried by a person: choosing keys in the
  launcher's window, and playing with a changed layout.
- The game reads and writes `[features]` through the same code as the launcher
  (`recomp-common/settings.h`); changing one in the menu leaves the rest of `settings.ini` as it was.

## Still emulated (what "native" has left to do)

1. (Done: calls and time slices.) The two CPUs no longer take turns every 32 cycles. Each runs up to
   the next hardware event (a few hundred cycles at most: a sound sample, a video line), the master
   first; only while one of them waits for the other (its registers come round to the same values
   with no memory write or hardware access by anyone in between) do they alternate in short slices,
   so that the one waiting sees what it waits for soon. At a sync point (game.cfg) a CPU hands over
   at once. `SATRECOMP_SHORT_SLICES=1`: every 32 cycles, as before.
   Translated code is no longer compared with memory at every entry: writes to memory that holds
   translated code are counted (in the translated code's stores, the bus and the file loader), and a
   comparison is only repeated after one (26 times fewer; `SATRECOMP_VERIFY_CODE=1` compares every
   time anyway and reports any change that went unnoticed: none in 12,000 frames each of two movies).
   Calls are made as calls: translated code calls the translation of its callee directly
   (runtime: `Call`), and a function goes on after the call inside itself. Because a time slice can
   end inside the callee, functions can be taken up again at any of their blocks: a caller left
   waiting is kept as a frame and goes on when control arrives at its return address, and a function
   that stopped at a backward branch goes on from there. The interpreter reference does all of this
   through the same code, so `make verify` compares as before (12,000 frames each of the two
   movies, 7,000 of the opening movie). A function can be entered at any of its blocks, so code
   arrived at in the middle of a function (after an interrupt, through a jump table) runs as that
   function too. Of the calls in a fight, 99% return into their caller: more than half straight
   away, the rest after a time slice ended inside the callee.
   `SATRECOMP_NO_CALLS=1`: every call through the dispatcher, as before.
2. (Done: no BIOS.) BIOS services nobody has called yet are not written; the port says so on stderr if
   one is called ("BIOS service at ... is not implemented").
3. (Done: CD, after start-up.) The game's use of the CD drive is replaced where it asks for it
   (once games/fighting_vipers/src/files.cpp and music.cpp; since 2026-11-01 the toolkit's
   satrecomp/runtime/shared_lifts.h, shared with Fighters Megamix, registered in src/game.cpp): its own LoadFile, the file library's
   Open/Start/Poll/Close, and the CD library's CdPlay, CdSeek and GetCurStat, with which it plays its
   music (audio tracks, played by runtime/music.cpp into the sound chip's CD input). The standard file
   library's Fread is the game's own and reads through the drive (satrecomp/machine/cd.cpp): the
   disc's directory at start-up, the sound program and samples before a fight, and the opening movie.
   (It was replaced until 2026-11-01; the replacement read at once, where the game waits in Fread
   with its interrupts running, and a fight's sound effects stopped after the first.) The SMPC
   is still emulated for what else the game asks of it (clock, system commands).
4. (Done: sound generation.) What the sound chip does to make each sample (voices, envelopes, LFOs,
   FM, the effects DSP, the mix) is satrecomp's (satrecomp/docs/SOUND.md), step for step what the
   reference gives. The chip's registers, timers and interrupts are the hardware model's. The 68000
   driver is translated, but through the interpreter's handlers (about 2% of run time).
5. (Done: video.) The chips' registers, memory and timing are still the hardware model's; the drawing
   is satrecomp's, in both modes (the reference renderer draws nothing unless asked to for a check).
   Left over: what Fighting Vipers does not use is not written (satrecomp/docs/VIDEO.md, "What is
   not done").
6. The SCU's DSP: the game never runs a program on it (`--stats` counts its instructions: none in any
   of the movies), so there is nothing to translate. (The SH-2 translation uses no interpreter
   handler.)
7. The SMPC: the peripheral library's request for controller data and system status, once a frame,
   is answered in its place (games/fighting_vipers/src/input.cpp, `SendCommand`; `FV_PER_CHECK=1`
   compares with the chip's answers: status and layout the same in all of 8,743 requests). What the
   chip still gets, by `--stats`: sound and second CPU on at start-up, the clock change (from the
   native BIOS), and reset button on/off some 30 times.

## Not done

- Coverage: the ending and some screens were never played by the explorers. Static discovery covers
  the code they lead to through fixed branches and calls, but not what is only reached through tables
  of addresses; that still runs in the interpreter until recorded (`--stats` shows how much and which).
- No optional features yet (the launcher's Features page is empty), no save
  states. Windows and Linux builds not started.
- Played by a person once (2026-10-10, app build): launches, works, looked accurate. Not a full playthrough.

## Notes on the game

- The two SH-2s share a command queue (queue base pointer at 0x0605A51C, read pointer at
  0x060FFC44, write pointer at 0x060FFC48). The slave's loop is at 0x0605B304 (command handlers in
  the table at 0x0605B360); idle, it keeps re-reading the first queue entry. The master's routine at
  0x0605CC70 resets the pointers and, about 20 instructions later, writes a command at the start of
  the queue. It relies on the slave running in between and noticing the reset; otherwise the slave is
  caught half way through the old entry, takes the new command's argument for a command, calls
  address 0 and ends in the BIOS exception loop (black screen after boot). The interpreter's 32-cycle
  slices happen to give the slave that time. Free running, the port does it on purpose: sync point
  0x0605CC7C in game.cfg (the master pauses there for 256 cycles).
- Wait loops: master 0x06057C5A (waits for a flag byte at GBR+19), slave 0x0605B346 (countdown
  between polls of the queue). Native code spends the rest of a time slice in them at once.
- First-read program loads at 0x06004000; code seen at 0x0600xxxx-0x0605xxxx and 0x060Bxxxx.

## Next

3. More coverage: a played-through movie, static discovery of branch targets.
4. Features behind flags, button remapping, save states (as in this author's other ports).
5. Video: colour calculation and the other effects Fighting Vipers does not use; the normal scroll
   screens drawn by the GPU too (the floor is).
6. Windows and Linux (SteamOS): built and replayed (docs/BUILDING.md); to be played by hand.

## Notes on the BIOS replacement

- After the BIOS has started the game, Fighting Vipers runs no BIOS ROM code except once: BUP_Init,
  which unpacks the backup library. Everything else it uses is the code the BIOS leaves in low work
  RAM (0x06000600-0x0600095F): the SCU interrupt entry/exit and the SYS_ services.
- The CD drive must be as the BIOS leaves it: disc authenticated, positioned at the start of the data
  track, paused, and the "disc changed" interrupt flag (HIRQ bit 5) cleared. With that flag still set
  the game's file system init fails and it calls SYS_EXECDMP.
- Services called by this game: SETSINT, SETSCUIM, CHGSCUIM (every frame), CHGSYSCK (once, to the
  352-pixel clock), BUP_Init and the backup library.

## Input

The game's pad routine (0x0602BA24) is lifted: both players' buttons come straight from the host's
controllers. The game's pad state is at 0x060943A0 (per player: held, newly pressed, held the frame
before; Saturn pad bit layout, 1 = pressed). The peripheral library the game links (0x06058000-
0x06058C00) asks the SMPC for controller data every frame and for the system status about once a
second (INTBACK), through one routine (0x06058934); the chip's answer comes by interrupt, and the
library's handler (0x0605D7E8) copies it to the buffers the pointers at 0x06058C10 name (status,
raw controller data, and the library's own digest of it). That request is answered natively: the
buffers are filled as the handler would, with the host's buttons, a clock that starts on 1 January
1994 and runs with the frames (so that runs repeat), and the status the chip gave.

## Lifted routines

| Address | Name | Source | What it does |
| --- | --- | --- | --- |
| 0x06029B44 | LoadFile(name, destination) | src/files.cpp | Loads a whole file by name and waits for it |
| 0x0602BA24 | ReadPads() | src/input.cpp | Takes both players' buttons and works out which are newly pressed |

A lift names the routine's address and a hash of its code (`satrecomp/tools/lifthash.py`), so it only
replaces the code it was written from. `SATRECOMP_NO_LIFTS=1` runs the original everywhere.
The characters' white (or green) flashes on hard hits are the game's own effect, not a loading fault
(checked by replaying a run with a screenshot every 25 frames: single frames only).

## The camera and the free camera (2026-10-31)

The view is made once a frame at 0x06005DEE (fv.h `camera`; r14 the camera's object): the library's unit matrix
(0x06052024), turned about x by 0x8000 less the object's word at +24 (0x0605280C) and about y by its word at +26
the other way round (0x0605289C), then moved by the eye (three longs, 16.16 metres, at 0x0605FBE0; x and y times
-300, z times 300: the hit task's convention) with 0x0605291C. The library's turns multiply on the right:
Rx = [1 0 0; 0 c -s; 0 s c], Ry = [c 0 s; 0 1 0; -s 0 c]. The camera's logic (from 0x06028700, by the mode at
0x060E0005) writes those values; it runs after the view is made, and works from what it wrote before (put
anything else there and the game's camera drifts, and with it the fighters' positions by rounding). The words at
0x0608034C/E are other angles that the sky and some effects turn by (0x06017A0E and its neighbours); the
16-byte records at 0x06080368 / 0x06080468 are a stage's colours (0x060393D0 makes a gradient from them).

`free_camera` (src/camera.cpp) puts its own pitch, yaw and eye in at the unit matrix's call (PR 0x06005DF6) and the
game's back at the move's (PR 0x06005E58), whose arguments are then in registers. Checked: with no turn the
picture is the game's, byte for byte; turned (120, 25 degrees, 1.6 times as far) the fighters, the hit task's
balls, the clock and the eye are the same in every frame of 6,000 as with it off (`SATRECOMP_PEEK` logs alike).
`FV_CAMERA=YAW,PITCH,DISTANCE` sets a turn for pictures with no window. Not seen in a window: turning it with a
stick or keys.

