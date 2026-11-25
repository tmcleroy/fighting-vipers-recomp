# Changelog

## Unreleased

- Which controller is which player: the launcher's Controllers page gives each connected gamepad Automatic, Player 1,
  Player 2 or Not used. On a Steam Deck, set its own controls to Not used and two gamepads plugged into it play each
  other. With nothing chosen, gamepads go to the players in the order they were connected, as before.
- The launcher's Game page has a link to the Discord server under Play: find players for a match, tell us what to fix
  or add, hear about new versions first. In the in-game menu, Reset all options asks "Are you sure?" first.

## 0.2.1

- .chd disc images: the game, the launcher's disc check and picker and the Android app read a
  `.chd` file as well as a `.cue` with its `.bin` files. A `.chd` holds every track in one file, the music's too (an
  `.iso` has only the data track, so no music). libchdr's license is in the download's licenses folder.
- The in-game menu is laid out as in the other recomp ports: Display and Game first, Speed and Mute on the Game
  page (with Cheats, Sound and Pictures under it), and a new **Reset all options** (every option back to the
  Enhanced preset, and full speed). The Armour page is under Training mode now. Speeds from 1/8 to 8x. The mouse works the menu; held arrows repeat;
  the gamepad's Guide button opens it (Back alone no longer does; Start and Back together still do). **P** pauses
  and **O** goes on a frame (F4 and F8 still do), **F12** saves a screenshot.
- Toolkit at satrecomp 0756c55.

## 0.2.0

- Android: an app for ARM64 phones and tablets, Android 9 and later (`FightingVipers-android.apk`). Touch controls beside
  the picture (a stick, the buttons and their pairs, Start and Menu) hide while a gamepad is used; Back opens the
  in-game menu. The disc is chosen with Android's picker, its .cue and all its .bin files at once, and copied into
  the app's folder. Played on a Pixel 9, widescreen and high resolution included (the GPU renderer no longer asks
  for GPU features it doesn't use, which Mali GPUs like the Pixel's lack).
- Honey's clothes: with clothes chosen, the computer's Candy in arcade mode (and team battle) was given them in
  the middle of being set up, and fought as her own model on the other clothes' data: with Aloha's, curled up in a
  ball, jumping about the arena, the camera close up on her. Only a Honey a player chooses wears them now; the
  computer's Candy is as in the original.
- Widescreen, the staff roll after arcade mode: rows of stripes showed at the sides from the sixth stage's scene on.
  The staff roll is drawn for 4:3 (each scene in a frame of its own), so it is shown in 4:3 now, black at the sides.
- The staff roll's song fades out over the winner's picture (Game, Enhanced), instead of going on at full volume
  into the GAME OVER music. The original game's command for the fade fades only CD music, and this song is not.
- Export pictures writes PNG files instead of JPEG: each picture exactly as the Saturn shows it. Pictures exported
  before stay as they were; export again for the PNG files.
- Toolkit at satrecomp 9616b00.

## 0.1.0-beta.5

Pepsiman, Honey's other clothes, the Portrait gallery and most of the exported pictures need the Japanese disc
image (Fighting Vipers (Japan)) beside your American one.

- Export pictures (in-game menu, Game → Pictures): every full-screen picture of the game as a JPEG file, 84 with the Japanese
  disc beside yours (the endings, the Portrait gallery, Pepsiman's, the Japanese versions), to the pictures folder
  in your files; Show pictures folder opens it.
- The winner's close-up after a fight: the winner was drawn mostly off the bottom of the screen. The game draws
  that shot in double interlace (twice the lines), which the GPU renderer drew at twice its height; it is drawn
  at its height now, as the software renderer and the Saturn draw it.
- Pepsiman, the Japanese release's extra fighter (Game, Enhanced): keep your Japanese disc image (Fighting Vipers
  (Japan)) in the same folder as the American one, or a folder in it, and the game takes his files from it; the
  launcher says under your disc whether it found it. He is unlocked as in Japan, or by Unlock everything; on the
  character select, left of Kumachan, left of Grace.
- Honey's clothes (Game): the Japanese release's other four (Pink, White ribbon, Uniform, Aloha with its
  ukulele attack), from the same disc, in arcade, versus, team battle and training.
- Portrait in Options Plus (Game, Enhanced): the Japanese release's picture viewer, the characters' pictures and
  a gallery of 22 more from the same disc; with Pepsiman unlocked, Options Plus's PEPSIMAN RESET row too.
- Unlock everything now shows Options Plus on the title's menu, as it said it did (the menu shows it only after
  arcade mode is finished, which the cheat did not count).
- The in-game menu as in the other ports: training mode's aids under Alternative modes → Training mode, the
  dummy's choices on a Dummy page under it, and Reset game at the bottom (the game starts again as when switched
  on; your saves are kept).
- The credits after the last arcade match no longer stop on their STAFF title: the game asks every frame after the
  file being streamed and took the "complete" the credits were waiting for. A background queue of files that
  loaded slowly in ordinary play for the same reason now loads at once.
- Widescreen: the training character select (its top row of faces cut apart, a purple bar showing through), the
  training's SELECT CPU TYPE and its display at the top were cut apart by the fight's gauge layout; that layout
  is now used in fights only, and not in training.
- Save states made by earlier versions can no longer be loaded (the toolkit's save states now keep the music's place).
- Toolkit at satrecomp 67a5fd4.

## 0.1.0-beta.4

- Sound effects in fights: fights had only the music and the round calls. The round's start hands the sound
  driver a request and waits for it to be taken; that code runs from the rewritten round code, where no time
  passed for the driver, so the game decided the driver was dead and turned effects off. The sound side now
  keeps up during such calls, and reads of the sound chip take as long as on the hardware. Toolkit at
  satrecomp 2086530.
- The training mode's input display no longer shows the stick at rest (5) as a line.

## 0.1.0-beta.3

- Unlock everything (Cheats): Mahler, B.M., Kumachan, Options Plus, Big Heads and the wall display, without
  playing for them. The game is shown its save with them unlocked; the save kept is your own (matches played
  still count), so with it off the game has what you have unlocked. From the next start or reset. Toolkit at
  satrecomp a7b9c65 (save unlocks).
- Toolkit at satrecomp a7b9c65: the file library's Fread is the game's own again, reading through the CD drive
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
