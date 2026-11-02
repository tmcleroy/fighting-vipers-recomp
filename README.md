# Fighting Vipers Recomp

An unofficial native PC port of *Fighting Vipers* (Sega AM2, Sega Saturn, 1996) for macOS, Linux, SteamOS and
Windows, made by static recompilation, with widescreen, high resolution and a training mode.

> **Beta.** Fights, menus and the opening movie run, but it has been tested far more by scripts than by hand, and on
> few machines and controllers. Expect rough edges, and please [report what you find](#feedback-and-contributing).

![The start of a round in 16:9 at the window's resolution](docs/screenshots/widescreen.jpg)

**No game files are included.** You need your own copy of the game: a disc image you made from your own *Fighting
Vipers* (USA) disc. No Saturn BIOS is needed.

The game's SH-2 code is translated ahead of time into C++ and runs on a small machine written for this port, with
its own replacement for the BIOS's services. **No emulator is part of the program.** Parts of the game (a round from
its set-up to who won it, damage, the clock, the input) have been rewritten by hand as readable C++, and the
features below live in that code. **Pure** plays the game exactly as it was on the Saturn; everything else is a
feature you switch on, in the launcher or in the menu while playing.

## Training mode

The game's own TRAINING MODE (on its main menu) gets the aids a fighting game's training wants. They work there and
stay off in the other modes.

- **Displays**: each fighter's inputs with frame counts, damage and hits in a row, frame data (the frame a move hit
  on, and the advantage after it), and what is left of each armour.
- **Move data**: the last attack's level, start-up, active and recovery frames, damage and advantage; every move
  done is kept in a table, and can be logged a line a move (`frame_data.csv`).
- **A hit overlay** of what the game tests for hits, and each fighter's **throw range**.
- **A dummy** that stands, crouches, guards, counters, jumps, struggles out of throws, gets up as you choose, or
  replays what you recorded.
- **A reset point** to go back to, pause, frame advance, slow motion, and gauges that refill.

![Training displays: inputs, armour, frame data and the hit overlay](docs/screenshots/training.jpg)

**[More about training mode →](docs/TRAINING.md)**

## The picture

- **High resolution**: polygons drawn by your graphics card at the window's resolution, not the Saturn's.
- **Widescreen**: fights in 16:9, with more of the arena at the sides and the gauges at the picture's edges.
- **A free camera**: the fight seen from any side, above or farther off. Only the picture changes.
- **The display your way**: sharp, whole-pixel or smooth scaling, 4:3 or square pixels, the window's size, and
  RetroArch's CRT and scanline shaders.

![A fight in 16:9: a hit taking armour off](docs/screenshots/fight.jpg)

**[More about the picture →](docs/PICTURE.md)**

## Everything else

- **Save states**: nine slots, from the menu or F5 / F9 ([keys](docs/LAUNCHER.md#keys-while-playing)).
- **Run-ahead**: what you press seen a frame or two sooner, the game running exactly as without it
  ([more](docs/LAUNCHER.md#run-ahead)).
- **Armour** that stays on, or fighters without it; **cheats** (no damage, one-hit knockouts, infinite time,
  everything unlocked without touching your save);
  **quick rounds** ([more](docs/LAUNCHER.md#the-features-page)).
- **Controls**: a gamepad or the keyboard for two players, every button changeable, and buttons pressed together on
  one key ([more](docs/LAUNCHER.md#controllers)).
- **Updates** in the launcher, and a **portable mode** that keeps everything beside the program ([the
  launcher](docs/LAUNCHER.md)).

## Installing

Download the file for your system from the [releases page](../../releases), then:

| System | File | Then |
| --- | --- | --- |
| macOS 13 or later, Apple silicon or Intel | `FightingVipers-macos.zip` | Unzip and open **Fighting Vipers.app**. It isn't notarized: the first time, right-click it and choose **Open** (or allow it under System Settings, Privacy & Security). |
| Linux x86_64, SteamOS (Steam Deck, Steam Machine) | `FightingVipers-linux-x86_64.tar.gz` | Unpack and run `fighting_vipers-launcher`. On SteamOS, do this in Desktop Mode, and use the launcher's **Add to Steam** to have it in Game Mode. |
| Linux ARM64 (aarch64) | `FightingVipers-linux-arm64.tar.gz` | Unpack and run `fighting_vipers-launcher`. |
| Windows 10 or later, x64 | `FightingVipers-windows-x64.zip` | Unzip and run `fighting_vipers-launcher.exe`. Windows warns about an unknown publisher the first time: choose **More info**, then **Run anyway**. |

In the launcher, drop your disc image's `.cue` file on the window (or choose it), choose **Features** and
**Controllers** as you like (or leave them), and **Play**. The launcher checks that the image is the release this
port is made from (USA, `MK-81041`, V2.000) and remembers where it is; it never copies, moves or changes it.

The disc image must be one you made from a disc you own. This project can't help you find game files, and requests
for them are closed. Where your settings and saves are kept: [docs/LAUNCHER.md](docs/LAUNCHER.md#your-files).

## Controls

Keyboard: arrows, Z / X / C for guard, punch and kick, Return for Start; on a gamepad, the d-pad or left stick and
the lower buttons. While playing, **Esc**, **F1** or the gamepad's Back button opens the menu (its "Quit game" row
is how you leave), and **F11** switches full screen. [docs/LAUNCHER.md](docs/LAUNCHER.md#controllers) has the whole
layout and every key.

## Status

| | |
| --- | --- |
| The game's code | Its SH-2 code translated to C++ ahead of time: 99.9% of what runs in a recorded match; the rest is one short piece of code at a single address, stepped by the port's own interpreter. The sound driver (the game's 68000 program) is run by the port's own 68000 interpreter; not translated yet. |
| The machine | Video, sound chip, timers, input, saves and BIOS services written for this port; no BIOS image needed. |
| Readable C++ | A round's states, damage, the clock, the fight's input ([games/fighting_vipers/PROGRESS.md](games/fighting_vipers/PROGRESS.md)). |
| macOS | Built and run on Apple silicon. The Intel half of the app replays a recorded match to the same state under Rosetta; not yet run on an Intel Mac. |
| Linux, SteamOS | x86_64: a recorded match replays to the same state, frame for frame, as on macOS, also on a Steam Deck, where the high-resolution renderer draws the same picture on Vulkan as on the Mac to within 0.2% of its pixels. ARM64: the same in an ARM64 container; not yet run on an ARM device. |
| Windows x64 | The same recorded match replays to the same state under Wine. Not yet run on a Windows PC. |

Known issues: only the USA release of the disc is supported; on Linux and Windows the high-resolution renderer needs
a Vulkan driver (without one the game falls back to the original resolution drawn in software); a Steam Deck has
room to spare but not much (measured without a window, about 80 frames a second in a fight in widescreen, about
70 with every training display on); the builds aren't signed or notarized, so macOS and Windows warn the first
time. [docs/ROADMAP.md](docs/ROADMAP.md) has what is planned.

## Building from source

You need your own disc image, a C++20 compiler (clang or GCC), CMake, Ninja, Python 3 and SDL 3.
[docs/BUILDING.md](docs/BUILDING.md) has the steps for each system, and [docs/TOOLKIT.md](docs/TOOLKIT.md) how the
port is made: the block database, the translator, the machine, the checks that compare each part with a reference.
`satrecomp/` is a toolkit for Saturn games in general, `games/fighting_vipers/` what is about this one game; another
Saturn game would be a second folder under `games/`.

## Feedback and contributing

Reports from real machines and controllers are the most useful thing right now. Please [open an issue](../../issues)
for bugs, crashes, glitches or ideas, with your system, your controller and which features were on.
[CONTRIBUTING.md](CONTRIBUTING.md) has the rules for code, the first of which is that nothing from the game ever
goes in the repository.

## Legal

This repository contains no game code, data, graphics, sound or text, no Sega BIOS, and nothing from Sega's
development kits. The block database and the C++ generated from the game are made on the builder's machine from
their own disc and are never committed. The release downloads contain the game's program code translated into native
code by the recompiler; everything else the game needs (models, textures, sound, music, the opening movie) is read
from your own disc image while you play. Please don't open issues or pull requests with game files or links to them.

[GNU GPL v3.0](LICENSE); third-party components and their licenses are in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). This project is not affiliated with, endorsed by, or sponsored by
Sega. *Fighting Vipers* and *Sega Saturn* are trademarks of Sega Corporation.

## Credits

- [Ymir](https://github.com/StrikerX3/Ymir) by StrikerX3 and contributors: the Saturn emulator every part of this
  port's machine was checked against during development, and the source of much of what is known here about the
  hardware.
- [wiiparty-recomp](https://github.com/arelkair/wiiparty-recomp) by arelkair: the launcher's look.
  [recomp-common](https://github.com/tmcleroy/recomp-common): the launcher, the in-game menu and the settings rules.
  [SDL](https://libsdl.org), [Dear ImGui](https://github.com/ocornut/imgui) and [Inter](https://rsms.me/inter/).
