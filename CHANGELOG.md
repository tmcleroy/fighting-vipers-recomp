# Changelog

## Unreleased

- Unlock everything (Cheats): Mahler, B.M., Kumachan, Options Plus, Big Heads and the wall display, without
  playing for them. The game is shown its save with them unlocked; the save kept is your own (matches played
  still count), so with it off the game has what you have unlocked. From the next start or reset. Toolkit at
  satrecomp 3100b56 (save unlocks).
- Toolkit at satrecomp c5cb660: the file library's Fread is the game's own again, reading through the CD drive
  (the toolkit's replacement read at once; in Fighters Megamix that stopped a fight's sound effects). The
  opening movie reads through the drive too. The widescreen cage limit is the toolkit's shared one now, with
  a wider margin. Known: a fight's sound effects are still missing (the music plays); the cause is in how the
  port loads files, not in the sound chip, and is being looked for.
- README: a short section per feature, with training mode, the picture and the launcher in docs/TRAINING.md,
  docs/PICTURE.md and docs/LAUNCHER.md.
- Toolkit at satrecomp 7b57029, from the Fighters Megamix movie fix: the CD drive goes on reading when
  sectors are taken out of a full buffer, and a CPU's loop that the other CPU overwrites keeps running from
  its cache. Fighting Vipers plays the same, state hash for hash.

## 0.1.0-beta.2

The first release from the public repository.

- Toolkit at satrecomp a29937a: the file, CD music and controller routines are the toolkit's shared ones
  (`satrecomp/runtime/shared_lifts.h`, also used by Fighters Megamix); the game plays the same, state hash
  for hash. The CD drive gives data sectors, cache purges leave memory alone, the SCU offers a higher
  interrupt over a lower one not yet taken. The audit passes again (librashader's header and its pinned
  shaders commit).
- A free camera (the Display page): the fight seen from wherever you turn it, round the two fighters, with a
  gamepad's right stick or I, J, K and L, nearer or farther with U and O. The game plays exactly the same.
- The Display page: Scaling (sharp, whole pixels, smooth), Picture shape (4:3, square pixels, fill), Window size,
  and Shader: CRT, CRT Lottes, CRT Royale, CRT Guest, CRT Light, scanlines (light, strong, OSSC, dark), xBRZ,
  ScaleFX or your own RetroArch preset, drawn by librashader at the Saturn's own resolution.
- Combined buttons on the Controllers page: punch + kick, guard + punch, guard + kick, guard + punch + kick.
- The launcher updates itself: it checks this repository's releases when it opens (the Updates card on
  its first page has the switch and Check now) and installs a newer version in place, keeping your
  settings and saves. It can find them once the repository is public. Its version is shown under its name.
- A download for Linux ARM64 (aarch64): `FightingVipers-linux-arm64.tar.gz`.
- Portable mode: an empty `portable.txt` beside the launcher (on macOS, beside the app) keeps your
  settings, saves and save states in a `user` folder there instead of your user folder. The launcher's
  first page says where they are, with Show to open the folder.
- The launcher: Play at the top of the first page; it fits a small window or large text; it is dark
  unless the system says light (so dark in Steam's Gaming Mode).
- The launcher no longer takes `.chd`, `.ccd` or `.mds` images, which the port cannot read.
- Toolkit at satrecomp 3490121 (recomp-common 28503a2).
- Run-ahead: off, 1 or 2 frames (the menu's Game page, or the launcher). What you press is seen a
  frame or two sooner; the game runs exactly as without it.
- The training aids work in the game's own TRAINING MODE, and stay off in the other modes (the menu
  and the settings keep what you chose). In the training mode they now also work at all: its fight
  is a state of its own, and its gauges take no damage (what a hit is worth is shown all the same).
- The dummy at random: its way up when knocked down, what it does as it gets up, and whether it
  struggles out of a throw; and "Dummy answers now and then", its counter and wake-up one time in two.
- Move log: every move done, a line of `frame_data.csv` beside the settings.
- Widescreen can be switched in the menu while playing (Display page); the launcher's switch follows
  it, and the window takes the picture's shape.

## 0.1.0-beta.1

The first public version.

- Fighting Vipers (USA) runs from your own disc image, with no Saturn BIOS: the game's SH-2 code
  translated to C++ ahead of time, on the port's own video renderer, sound chip and machine.
- A launcher: your disc image, features (Pure, Enhanced or your own choice), keys and gamepad
  buttons for each player.
- High resolution and 16:9 widescreen.
- A menu while playing: save states, the features' pages, speed, a reset point.
- Training: input display, damage and frame data readouts, the hit overlay, armour options, a dummy
  that guards, punches back, jumps or replays a recording, and how it gets up.
- Cheats: no damage, one-hit knockouts, infinite time, quick rounds.
- Builds for macOS (Apple silicon and Intel), Linux x86_64 (SteamOS) and Windows x64.
- The launcher can be used with a gamepad alone, or with the arrow keys and Space or Enter: a ring
  shows what is chosen, A presses it, B goes back, the shoulder buttons change the page. With a
  gamepad, Choose opens a list of folders and files in the launcher's own window.
- In the in-game menu Space, Enter and a gamepad's A choose and Backspace and B go back, whatever
  the game's buttons are set to.
- Sound can be switched off: a "Muted" switch in the launcher (off unless switched on) and a
  "Sound" row in the in-game menu.
- The `satrecomp` toolkit has a [repository of its own](https://github.com/tmcleroy/satrecomp); `satrecomp/` here is a copy of it, made by
  `satrecomp/tools/sync-toolkit.sh` and checked by `make check-toolkit` (part of `make check`,
  `make check-rewrites` and CI). `make SR=../satrecomp` builds the game against a clone of the
  toolkit, to `build/fighting_vipers-dev`.
