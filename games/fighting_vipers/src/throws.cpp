// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
// Fighting Vipers: requests and throws, rewritten from the game's code as readable C++
// (satrecomp/docs/READABLE.md; each routine here is checked against the game's own, as those of fight.cpp are).
//
// A fighter is asked to do something by a request (fv.h, fighter::kRequest): the reading of its commands
// leaves one each frame, and the game's own routines leave others that go before it. A throw is a request
// of the thrower's that the game answers with a request to the other: to be thrown. That answer is all
// there is to a throw's success here; whether the throw could be asked for at all (the reach, how the other
// stands) was settled when the command was read, by the bits of fighter::kReach.
#include "fv.h"
#include "game.h"
#include "lift.h"

namespace {

using satrecomp::Guest;
using namespace fv;

constexpr uint32_t kPendingRequest = 0x0600CB8E, kMoveRecord = 0x06012428;
constexpr uint32_t kArmourOffByThrow = 0x06011838; // (not rewritten; what it does was not looked into)

// @replaces 0x06012428
// MoveRecord(r4 = move, r5 = type): the record of that type in the move's data (fv.h, `move`), or 0.
bool MoveRecord(Guest &g) {
    uint32_t at = g.Read32(move::kTable + ((g.R(4) & 0x0FFF) << 2)) + 8;
    uint32_t size = 0x0FFF; // (left in r1 when the first record is the one)
    int32_t type;
    for (;;) {
        type = Byte(g, at);
        if (static_cast<uint32_t>(type) == g.R(5)) {
            break;
        }
        if (type == 0 || type == 8) { // (the end of the records)
            at = 0;
            break;
        }
        size = static_cast<uint32_t>(static_cast<int32_t>(Byte(g, move::kRecordSizes + static_cast<uint32_t>(type))));
        at += size;
    }
    g.R(0) = at;
    g.R(1) = size; // left
    g.R(3) = static_cast<uint32_t>(type);
    g.R(4) = at;
    g.Return(60);
    return true;
}

// @replaces 0x0600CB8E
// PendingRequest(r12 = fighter): which request the fighter has this frame. One the game forced goes before
// the commands'; so does one put off until a frame of the move, when that frame has come. Otherwise the
// commands' request stands, unless the fighter takes no commands now (it is being thrown, say), when only
// two kinds are let through.
bool PendingRequest(Guest &g) {
    const uint32_t fighter = g.R(12);
    uint32_t request = g.Read32(fighter + fighter::kForcedRequest);
    if (request == 0) {
        request = g.Read32(fighter + fighter::kDeferredRequest);
        if (request != 0 && g.Read16(fighter + fighter::kDeferredUntil) != g.Read16(fighter + fighter::kMoveFrame)) {
            request = 0;
        }
    }
    if (request != 0) {
        g.Write32(fighter + fighter::kRequest, request);
        g.Write32(fighter + fighter::kDeferredRequest, 0);
    } else {
        const int8_t kind = Byte(g, fighter + fighter::kRequest);
        bool dropped;
        if (kind == 0x1C) { // (a request that only moves the move on three frames)
            g.Write16(fighter + fighter::kMoveFrame, static_cast<uint16_t>(g.Read16(fighter + fighter::kMoveFrame) + 3));
            dropped = true;
        } else {
            dropped = (g.Read8(fighter + fighter::kStatus) & fighter::kNoCommands) && kind != 0x0D && kind != 0x23;
        }
        if (dropped) {
            g.Write32(fighter + fighter::kRequest, 0);
        } else if (g.Read32(fighter + fighter::kRequest) != 0) {
            g.Write32(fighter + fighter::kDeferredRequest, 0); // (a new request: what was put off is forgotten)
        }
    }
    g.Return(120);
    return true;
}

// @replaces 0x0600C974
// AnswerThrows(r12 = a fighter, r13 = the other): each fighter's request for the frame, and then, if one of
// them asks for a throw, the other's request becomes to be thrown by it. Four kinds of throw are answered,
// in this order; if both fighters ask for the same kind on the same frame, the round's clock decides whose
// it is (an even count: the first fighter's).
bool AnswerThrows(Guest &g) {
    const uint32_t first = g.R(12), second = g.R(13);
    g.R(10) = first; // left (the callers read both)
    g.R(11) = second;
    g.Call(kPendingRequest);
    g.R(12) = second;
    g.R(13) = first;
    g.Call(kPendingRequest);

    static constexpr struct {
        int8_t asked;    // the thrower's request
        uint32_t answer; // the other's, to which the move of being thrown is added
    } kThrows[] = {{0x27, 0x28002000}, {0x25, 0x26002000}, {action::kThrow, static_cast<uint32_t>(action::kThrown) << 24}, {0x2B, 0x2C000000}};

    // (not while a throw is the request the game forced on either)
    if (Byte(g, first + fighter::kForcedRequest) != action::kThrow && Byte(g, second + fighter::kForcedRequest) != action::kThrow) {
        const int8_t firstAsks = Byte(g, first + fighter::kRequest), secondAsks = Byte(g, second + fighter::kRequest);
        g.R(7) = static_cast<uint32_t>(static_cast<int32_t>(firstAsks)); // (as the game's code has them where it calls on)
        g.R(8) = static_cast<uint32_t>(static_cast<int32_t>(secondAsks));
        for (const auto &kind : kThrows) {
            if (firstAsks != kind.asked && secondAsks != kind.asked) {
                continue;
            }
            const bool bySecond = firstAsks != kind.asked || (secondAsks == kind.asked && (g.Read32(kRoundTime) & 1));
            uint32_t thrower = bySecond ? second : first, thrown = bySecond ? first : second;
            g.R(12) = thrower;
            g.R(13) = thrown;
            if (kind.asked == 0x2B && (g.Read8(thrown + fighter::kStatus) & fighter::kKnockedOut)) {
                // (this kind is not done to a fighter already knocked out: the thrower is let go)
                g.Write32(thrower + fighter::kRequest, static_cast<uint32_t>(action::kNone) << 24);
                g.Write32(thrower + fighter::kState, g.Read32(thrower + fighter::kState) & ~fighter::kBusy);
                break;
            }
            // The throw's record says which move being thrown by it is, and what it takes off the gauge
            g.R(4) = static_cast<uint32_t>(static_cast<int32_t>(static_cast<int16_t>(g.Read16(thrower + fighter::kRequest + 2))));
            g.R(5) = 5;
            g.Call(kMoveRecord);
            g.R(9) = g.R(0); // left
            g.Write32(thrown + fighter::kRequest, static_cast<uint32_t>(static_cast<int32_t>(static_cast<int16_t>(g.Read16(g.R(9) + 2)))) + kind.answer);
            if (kind.asked == 0x25 && !(g.Read32(thrown + fighter::kState) & 0x1000)) {
                g.R(12) = thrown;
                g.R(13) = thrower;
                g.Call(kArmourOffByThrow);
                thrown = g.R(12);
                thrower = g.R(13);
                g.Write32(thrown + fighter::kState, g.Read32(thrown + fighter::kState) | 0x1000);
            }
            // A fighter thrown while it could not act takes a quarter more
            uint32_t power = static_cast<uint32_t>(static_cast<int32_t>(Byte(g, g.R(9) + 1)));
            if (g.Read32(thrown + fighter::kState) & fighter::kBusy) {
                power += power >> 2;
            }
            g.Write8(thrown + fighter::kAttackPower, static_cast<uint8_t>(power));
            g.Write32(thrown + fighter::kState, g.Read32(thrown + fighter::kState) & ~0x00200000u);
            // (which way round the thrown fighter is, from the thrower's)
            const uint32_t turned = ((g.Read32(thrower + fighter::kState) >> 16) << 1) ^ g.Read32(thrower + fighter::kFlags);
            const uint32_t flags = g.Read32(thrown + fighter::kFlags);
            g.Write32(thrown + fighter::kFlags, turned & 0x40 ? flags | 0x40 : flags & ~0x40u);
            break;
        }
    }
    g.Write32(g.R(10) + fighter::kForcedRequest, 0);
    g.Write32(g.R(11) + fighter::kForcedRequest, 0);
    g.R(12) = first;
    g.R(13) = second;
    g.Return(400);
    return true;
}

const satrecomp::Lift kLifts[] = {
    // (free: what the one caller, 0x0600C786, does not read after it)
    {"AnswerThrows", 0x0600C974, 0x21A, 0x05ECA32EA2E4DF3Bull, AnswerThrows, true, 0x101FF},
    {"PendingRequest", kPendingRequest, 0x74, 0x708390332921C871ull, PendingRequest, true, 0x130FF},
    // (free: the T flag only; its thirteen callers read most of what it leaves)
    {"MoveRecord", kMoveRecord, 0x36, 0x30F184673E3BAE2Dull, MoveRecord, true, 0x10000},
};

} // namespace

namespace satrecomp {
void RegisterThrowLifts() {
    RegisterLifts(kLifts, sizeof(kLifts) / sizeof(kLifts[0]));
}
} // namespace satrecomp
