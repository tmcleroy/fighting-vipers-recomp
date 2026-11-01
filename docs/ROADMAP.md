# Roadmap

What is planned, roughly in order. [games/fighting_vipers/PROGRESS.md](../games/fighting_vipers/PROGRESS.md)
has the detail of what is known about the game and what is still its own code.

## Before leaving beta

- Every platform's download played by hand, with a gamepad and the keyboard: macOS, a Steam Deck,
  a Windows PC.
- The high-resolution renderer seen in a window on Vulkan (Linux, Windows): it has been run on a Steam
  Deck's Vulkan driver without a window and compared with the Mac's pictures, not watched.
- The whole arcade mode played through, and versus with two pads.

## The port

- The sound driver translated ahead of time, as the game's main code is (it runs in the port's own
  68000 interpreter now).
- More of the game rewritten as readable code, led by what features need: the reading of commands,
  a fighter's own frame, moves and hits.
- The remaining hardware models replaced by the game's own calls answered directly (as file loading
  and the BIOS's services already are).
- An AppImage for Linux; Linux on arm64.

## Features

- Training: throws and move lists, more dummy behaviours, frame data checked against published tables.
- The other releases of the disc (Japan, Europe).
- The normal scroll screens drawn by the GPU as well (the floor already is).

## The toolkit

`satrecomp/` is written so that a second Saturn game is a folder under `games/`. None has been
started.
