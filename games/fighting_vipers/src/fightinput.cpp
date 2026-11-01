// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
// Fighting Vipers: what the fighters are told to do, rewritten from the game's code as readable C++
// (satrecomp/docs/READABLE.md; see fight.cpp for what the lines marked "left" are).
//
// Once a frame, for each player: the pad's buttons become the fighter's input (where a person plays), and
// the input step keeps the history of it that commands are read from, and says which way is forward. The
// rest of the input step, the reading of commands (0x06014FFC on), is still the game's code.
#include "fv.h"
#include "game.h"
#include "lift.h"
#include "training.h"

namespace {

using satrecomp::Guest;
using namespace fv;

// The input a pad's buttons give, by the player's button settings
uint8_t PadInput(Guest &g, int player) {
    uint32_t buttons = static_cast<uint32_t>(g.Read16(kPads + static_cast<uint32_t>(player) * pad::kSize + pad::kHeld)) >> 3, bits = 0;
    for (uint32_t at = input::kButtonTable[player];; at += 4, buttons >>= 1) {
        const int32_t gives = static_cast<int32_t>(g.Read32(at));
        if (gives < 0) {
            break;
        }
        if (buttons & 1) {
            bits |= static_cast<uint32_t>(gives);
        }
    }
    return static_cast<uint8_t>(bits);
}

// The history's readers all start from the same entry
void SetReaders(Guest &g, uint32_t fighter, uint8_t entry) {
    for (uint32_t reader = 1; reader < 4; reader++) {
        g.Write8(fighter + fighter::kHistoryReaders + reader, entry);
    }
    g.R(0) = fighter::kHistoryReaders + 3; // left
}

// @replaces 0x06014D44 as far as 0x06014FFC (r13: the fighter, r14: the player's record)
// The input step: counts two timers down; starts the reading of commands afresh where the fighter's state
// asks for it; puts the frame's input into the history, with what was pressed and let go of since the
// frame before; and works out which way is forward. Then the game's code reads commands from the history.
void InputStep(Guest &g) {
    const uint32_t fighter = g.R(13), player = g.R(14);
    const auto word = [&](uint32_t offset) { return static_cast<int16_t>(g.Read16(fighter + offset)); };

    if (g.Read32(fighter + fighter::kInputFlags) & 0x08) {
        // The input starts afresh: nothing in progress, and a history of one entry, this frame's input
        g.Write32(fighter + fighter::kInputFlags, 1);
        g.Write32(fighter + fighter::kCommand, 0);
        g.Write32(fighter + 0x1244, 0);
        g.Write32(fighter + 0x1258, 0);
        g.Write16(fighter + 0x123A, 0);
        g.Write16(fighter + 0x1260, 0);
        g.Write32(fighter + fighter::kForward, 0);
        g.Write32(fighter + fighter::kBackward, 0);
        g.Write8(fighter + fighter::kHistoryAt, 1);
        g.Write8(fighter + fighter::kHistoryReaders, 1);
        const uint8_t now = g.Read8(fighter + fighter::kInput);
        g.Write8(fighter + fighter::kHistory + input::kHeld, now);
        g.Write8(fighter + fighter::kHistory + input::kHeldOrLastDirection, now);
        SetReaders(g, fighter, 1);
        g.R(3) = 1; // left
        return;
    }

    // Two counts that run down (what they time is not known)
    if (g.Read32(fighter + 0x1244) != 0) {
        if (word(0x1248) != 0) {
            g.Write16(fighter + 0x1248, static_cast<uint16_t>(word(0x1248) - 1));
        } else {
            g.Write32(fighter + 0x1244, 0);
        }
    }
    if (word(0x1260) != 0) {
        g.Write16(fighter + 0x1260, static_cast<uint16_t>(word(0x1260) - 1));
    }

    // The reading of commands goes back to the start of the player's table unless a command is in progress
    // (and always while bit 10 of the fighter's flags is set)
    const uint32_t flags = g.Read32(fighter + fighter::kFlags), state = g.Read32(fighter + fighter::kState);
    uint32_t command = g.R(10);
    bool marked = false, fromTheStart;
    if (flags & 0x400) {
        marked = true;
        g.Write16(fighter + 0x123A, 0);
        fromTheStart = true;
    } else {
        command = g.Read32(fighter + fighter::kCommand);
        marked = state & 1;
        fromTheStart = !marked && !(command != 0 && word(0x123A) != 0);
    }
    if (fromTheStart) {
        command = player::kCommandTables[Byte(g, player + player::kNumber) != 0];
        g.Write32(player + player::kCommands, command);
        g.Write8(player + 46, 0);
        g.Write32(fighter + fighter::kCommand, 0);
        g.Write16(fighter + 0x1260, 0);
    }
    uint32_t inputFlags = g.Read32(fighter + fighter::kInputFlags);
    inputFlags = inputFlags & 0x02 ? inputFlags | 0x04 : inputFlags & ~0x04u;
    inputFlags = marked ? inputFlags | 0x02 : inputFlags & ~0x02u;
    g.Write32(fighter + fighter::kInputFlags, inputFlags);

    // The history: this frame's entry, from the input and the entry before
    const uint32_t at = g.Read8(fighter + fighter::kHistoryAt);
    const uint32_t entry = fighter + fighter::kHistory + at * 4, before = fighter + fighter::kHistory + ((at - 1) % input::kHistoryFrames) * 4;
    const uint8_t now = g.Read8(fighter + fighter::kInput);
    const uint8_t was = g.Read8(before + input::kHeld), wasDirection = g.Read8(before + input::kHeldOrLastDirection);
    const uint8_t pressed = now & ~was, released = was & ~now;
    g.Write8(player + player::kReleased, released);
    g.Write8(entry + input::kHeld, now);
    g.Write8(entry + input::kPressed, pressed);
    g.Write8(entry + input::kReleased, released);
    // (with no left or right held, the last one held stays; the same for up and down)
    uint8_t direction = now;
    if (!(direction & (input::kLeft | input::kRight))) {
        direction |= wasDirection & (input::kLeft | input::kRight);
    }
    if (!(direction & (input::kUp | input::kDown))) {
        direction |= wasDirection & (input::kUp | input::kDown);
    }
    g.Write8(entry + input::kHeldOrLastDirection, direction);
    // The ring goes on; a reader it has caught up with is pushed along
    const uint8_t next = static_cast<uint8_t>((at + 1) % input::kHistoryFrames), afterNext = static_cast<uint8_t>((next + 1) % input::kHistoryFrames);
    g.Write8(fighter + fighter::kHistoryAt, next);
    for (uint32_t reader = 1; reader < 4; reader++) {
        if (g.Read8(fighter + fighter::kHistoryReaders + reader) == next) {
            g.Write8(fighter + fighter::kHistoryReaders + reader, afterNext);
        }
    }

    // Which way is forward: from the side the fighter is on, which also goes into the input (unless the
    // input came with it)
    bool onTheRight;
    if (g.Read32(player + player::kFlags) & 0x04) {
        onTheRight = now & input::kOnTheRight;
    } else {
        onTheRight = flags & fighter::kOnTheRight;
        g.Write8(fighter + fighter::kInput, onTheRight ? now | input::kOnTheRight : now & ~input::kOnTheRight);
    }
    training::InputSeen(Byte(g, player + player::kNumber) != 0, now, onTheRight);
    const uint32_t forward = onTheRight ? input::kLeft : input::kRight, backward = onTheRight ? input::kRight : input::kLeft;
    g.Write32(fighter + fighter::kForward, forward);
    g.Write32(fighter + fighter::kBackward, backward);

    // Presses counted while the fighter is in some state (bit 0 with bit 18 or 19 of its state)
    if ((state & 1) && (state & 0x000C0000)) {
        if (pressed) {
            g.Write16(fighter + fighter::kPressesCounted, static_cast<uint16_t>(word(fighter::kPressesCounted) + 1));
        }
    } else {
        g.Write16(fighter + fighter::kPressesCounted, 0);
    }

    g.R(6) = forward; // left (the rest reads these)
    g.R(7) = backward;
    g.R(8) = now;
    g.R(9) = pressed;
    g.R(10) = command;
    g.R(11) = marked;

    // A press in another state (bits 0 and 24) asks for something of the fighter (0x1C000000 at 0x3C of its
    // record), at once with bit 19 set or once the words at 0x648 and 0x52 are far enough apart; and the
    // history's readers start from this frame
    if (pressed && (state & 1) && (state & 0x01000000)) {
        if ((state & 0x00080000) || word(0x4C6) * 2 <= word(0x648) - word(0x52)) {
            g.Write32(fighter + 0x3C, 0x1C000000);
            g.R(3) = 0x1C000000; // left
            g.Write8(fighter + fighter::kHistoryReaders, next);
            SetReaders(g, fighter, next);
            return;
        }
    }
    g.Call(0x06014FFC); // the reading of commands
}

// @replaces 0x06014C44 (player 1), 0x06014CC4 (player 2)
// A player's input for the frame. Where a person plays, the fighter's input is what the pad's buttons give
// (nothing, if the player may not act: the round is lost). Then the input step.
void PlayerInput(Guest &g, int number) {
    const uint32_t player = number ? kPlayer2 : kPlayer1, fighter = number ? kFighter2 : kFighter1;
    const uint32_t bit = number ? 0x02 : 0x01;
    g.R(14) = player; // (as the game's code has them for the input step)
    g.R(13) = fighter;
    g.R(12) = number ? kFighter1 : kFighter2;
    if (!(g.Read32(input::kNotFromPad[number]) & 1)) {
        const uint32_t flags = g.Read32(player + player::kFlags);
        uint8_t now = PadInput(g, number);
        if (!(flags & bit)) {
            now = 0;
        }
        bool played = !(flags & 0x04) && (g.Read8(kPlayers) & bit);
        // (the training dummy, and the player while it is recorded: training.cpp)
        if (const int dummy = training::Input(g, number, now); dummy >= 0) {
            now = static_cast<uint8_t>(dummy);
            played = true;
        }
        if (played) {
            g.Write8(fighter + fighter::kInput, now);
        }
    }
    g.Write8(player + player::kNumber, static_cast<uint8_t>(number));
    // (The game's routine has its return address on the stack and keeps the stack pointer in the player's
    // record over the input step, to put it back after)
    const uint32_t stack = g.R(15) - 4;
    g.Write32(stack, g.PR());
    g.R(15) = stack;
    g.Write32(player + player::kStack + static_cast<uint32_t>(number) * 4, stack);
    InputStep(g);
    g.R(15) = g.Read32(player + player::kStack + static_cast<uint32_t>(number) * 4) + 4;
}

// @replaces 0x06014C06 (on the task list during a fight)
bool InputTask(Guest &g) {
    if (!(g.Read32(kFightFlags) & kFightPausedBit)) {
        g.R(15) -= 4; // (the game's routine has its return address on the stack meanwhile)
        g.Write32(g.R(15), g.PR());
        PlayerInput(g, 0);
        PlayerInput(g, 1);
        g.R(15) += 4;
    }
    g.Return(200);
    return true;
}

bool PlayerInput1(Guest &g) {
    PlayerInput(g, 0);
    g.Return(100);
    return true;
}
bool PlayerInput2(Guest &g) {
    PlayerInput(g, 1);
    g.Return(100);
    return true;
}
bool InputStepAlone(Guest &g) {
    InputStep(g);
    g.Return(100);
    return true;
}

// (free: from tools/liveout.py for each)
const satrecomp::Lift kLifts[] = {
    {"InputTask", 0x06014C06, 0x20, 0x80B5E5D18826C40Aull, InputTask, true, 0x1433F},
    {"PlayerInput1", 0x06014C44, 0x80, 0x96DFA3700C3227D6ull, PlayerInput1, true, 0x170F7},
    {"PlayerInput2", 0x06014CC4, 0x80, 0x9D006AADBC28D968ull, PlayerInput2, true, 0x100FE},
    {"InputStep", 0x06014D44, 0x2BA, 0xBBA5EA58675A7310ull, InputStepAlone, true, 0x100FE},
};

} // namespace

namespace satrecomp {
void RegisterFightInputLifts() {
    RegisterLifts(kLifts, sizeof(kLifts) / sizeof(kLifts[0]));
}
} // namespace satrecomp
