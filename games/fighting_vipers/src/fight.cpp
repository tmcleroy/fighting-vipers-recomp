// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
// Fighting Vipers: fight rules, rewritten from the game's code as readable C++ (satrecomp/docs/READABLE.md).
//
// Each routine here does exactly what the game's routine at its address does: SATRECOMP_LIFT_CHECK=1 runs
// both for every call and compares memory and registers. The game's code is hand-written assembly, whose
// callers may go on to use what a routine happens to leave in a register; so a routine here ends by leaving
// those registers as the game's does (the lines marked "left"), unless no caller reads them.
//
// The cheats (cheats.cpp) are here too, as a few lines in the rules they change, and the training aids
// (training.cpp) are told of hits and frames from here. With one of them switched on a routine no longer
// does what the game's does, so the check is for runs without them.
#include "fv.h"
#include "game.h"
#include "lift.h"
#include "training.h"

namespace {

using satrecomp::FeatureOn;
using satrecomp::Guest;
using namespace fv;

// @replaces 0x060117D4 (a hit: also noted as the damage of the hit being taken), 0x060117D0 (other damage)
// TakeDamage(r12 = fighter, r4 = damage): takes damage off a fighter's gauge and knocks them out when
// nothing is left. Nothing comes off once the round's time is up, or from a fighter already knocked out,
// or while the fighter is marked as taking no damage.
bool TakeDamage(Guest &g, bool hit) {
    const uint32_t fighter = g.R(12);
    const uint32_t damage = g.R(4);
    const uint32_t statusAt = fighter + fighter::kStatus;

    g.Write8(fighter + fighter::kHitFlags, g.Read8(fighter + fighter::kHitFlags) & ~1u);
    g.Write16(fighter + fighter::kLastDamage, static_cast<uint16_t>(damage));

    const int32_t timeLeft = static_cast<int32_t>(g.Read32(kRoundTime));
    const int8_t status = static_cast<int8_t>(g.Read8(statusAt));
    // (the training mode's fight takes nothing off a gauge: what the hit is worth is told to the training aids
    // all the same, as the game's own DAMAGE display shows it)
    const bool inTraining = g.Read8(kGameMode) == kModeTraining;
    g.R(3) = hit ? 0 : 1; // left
    if (timeLeft <= 0) {
        if (inTraining) {
            training::Damage(fighter == kFighter2, static_cast<int32_t>(damage));
        }
        g.R(0) = fighter::kLastDamage; // left
        g.R(1) = static_cast<uint32_t>(timeLeft);
    } else if (status & fighter::kKnockedOut) {
        g.R(0) = static_cast<uint32_t>(static_cast<int32_t>(status)); // left
        g.R(1) = statusAt;
    } else {
        if (hit) {
            g.Write16(fighter + fighter::kHitDamage, static_cast<uint16_t>(damage));
        }
        const int8_t noDamage = static_cast<int8_t>(g.Read8(fighter + fighter::kNoDamage));
        if (noDamage != 0) {
            if (inTraining) {
                training::Damage(fighter == kFighter2, static_cast<int32_t>(damage));
            }
            g.R(0) = fighter::kNoDamage; // left
            g.R(1) = static_cast<uint32_t>(static_cast<int32_t>(noDamage));
        } else {
            const int32_t gaugeBefore = static_cast<int16_t>(g.Read16(fighter + fighter::kGauge));
            int32_t taken = static_cast<int32_t>(damage);
            if (FeatureOn(fighter == kFighter1 ? "no_damage" : "no_damage_2")) {
                taken = 0;
            } else if (fighter == kFighter2 && FeatureOn("one_hit")) {
                taken = gaugeBefore;
            }
            training::Damage(fighter == kFighter2, taken);
            const int32_t gauge = gaugeBefore - taken;
            if (gauge > 0) {
                g.Write16(fighter + fighter::kGauge, static_cast<uint16_t>(gauge));
                g.R(1) = 0; // left
                g.R(3) = static_cast<uint32_t>(gauge);
            } else {
                g.Write8(statusAt, static_cast<uint8_t>(status | fighter::kKnockedOut));
                g.Write16(fighter + fighter::kGauge, 0);
                g.R(1) = statusAt; // left
                g.R(3) = 0;
            }
            g.R(0) = fighter::kGauge; // left
        }
    }
    g.Return(30);
    return true;
}

// @replaces 0x06024D58
// Adds a frame to the time the round has taken, in 1/100 seconds: 100/64 a frame, by a table of sixteen
// steps of 1 and 2.
void CountTimeTaken(Guest &g) {
    int8_t step = static_cast<int8_t>(g.Read8(kTimeTakenStep));
    if (step <= 0) {
        step = 16;
    }
    step--;
    g.Write8(kTimeTakenStep, static_cast<uint8_t>(step));
    g.Write32(kTimeTaken, g.Read32(kTimeTaken) + static_cast<int8_t>(g.Read8(kCentisecondSteps + step)));
    g.R(0) = kCentisecondSteps; // left
    g.R(6) = kTimeTakenStep;
    g.R(7) = kTimeTaken;
}

bool CountTimeTakenAlone(Guest &g) {
    CountTimeTaken(g);
    g.Return(20);
    return true;
}

// @replaces 0x06024CFC (on the game's task list during a round)
// The round's clock, once a frame: counts the time left down and has it drawn; when none is left, says so
// (and stops counting the time taken).
bool RoundClock(Guest &g) {
    if (g.Read8(kFightPaused) & kFightPausedBit) {
        g.Return(20);
        return true;
    }
    const uint8_t switches = g.Read8(kClockSwitches);
    if (switches & kClockCountsDown) {
        const int32_t timeLeft = static_cast<int32_t>(g.Read32(kRoundTime));
        if (timeLeft <= 0) {
            g.Write8(kRoundEvents, g.Read8(kRoundEvents) | kTimeUp);
            g.R(6) = static_cast<uint32_t>(timeLeft); // left
            g.R(7) = kRoundTime;
            g.Return(40);
            return true;
        }
        if (!FeatureOn("infinite_time")) {
            g.Write32(kRoundTime, static_cast<uint32_t>(timeLeft - 1));
        }
        g.Call(kDrawRoundTime);
    }
    if (g.Read8(kClockSwitches) & kClockCountsTimeTaken) {
        CountTimeTaken(g);
    }
    g.Return(60);
    return true;
}

// @replaces 0x0600FE80 (on the task list during a fight)
// The fighters, once a frame: each one's own frame (the game's code has the fighter it works on in r12 and
// the other in r13), then what each does to the other; and the count at 0x060E0022 goes down.
bool FightersTask(Guest &g) {
    g.R(10) = task::kFighter1; // left
    g.R(11) = task::kFighter2;
    const uint32_t on1 = g.Read32(g.R(10) + task::kOn), on2 = g.Read32(g.R(11) + task::kOn);
    g.R(12) = g.Read32(g.R(10));
    g.R(13) = g.Read32(g.R(11));
    g.R(7) = on1;
    g.R(8) = on2;
    if (on1 == 1) {
        g.Call(kFighterFrame);
    }
    g.R(12) = kFighter2;
    g.R(13) = kFighter1;
    if (on2 == 1) {
        g.R(7) = on1;
        g.R(8) = on2;
        g.Call(kFighterFrame);
    }
    g.R(7) = on1;
    g.R(8) = on2;
    if ((on1 & on2) == 1) {
        g.R(12) = kFighter1;
        g.R(13) = kFighter2;
        g.Call(kFighterAgainstOther);
        g.R(12) = kFighter2;
        g.R(13) = kFighter1;
        g.Call(kFighterAgainstOther);
    }
    const bool paused = g.Read32(kFightFlags) & kFightPausedBit;
    training::Frame(g, paused);
    if (!paused) {
        g.Write8(g.R(12) + fighter::kInput, 0);
        g.Write8(g.R(13) + fighter::kInput, 0);
        const uint8_t count = g.Read8(kNoThrowsFor);
        if (count != 0) {
            g.Write8(kNoThrowsFor, count - 1);
        }
    }
    g.Return(200);
    return true;
}

const satrecomp::Lift kLifts[] = {
    {"FightersTask", 0x0600FE80, 0xA4, 0xB9B1490AC6A96AC0ull, FightersTask, true, 0x1433F},
    // (free: what the task list's caller and the round clock do not read after them)
    {"RoundClock", 0x06024CFC, 0x52, 0x4D780469A6CC6D30ull, RoundClock, true, 0x1003F},
    {"CountTimeTaken", 0x06024D58, 0x1C, 0xC5487F15E206F865ull, CountTimeTakenAlone, true, 0x100FE},
    // (free: the T flag, which no caller reads)
    {"TakeDamage (other)", 0x060117D0, 0x58, 0x8B22B8E711E14F40ull, [](Guest &g) { return TakeDamage(g, false); }, true, 0x10000},
    {"TakeDamage (hit)", 0x060117D4, 0x54, 0xF5A4D91724AC3A8Bull, [](Guest &g) { return TakeDamage(g, true); }, true, 0x10000},
};

} // namespace

namespace satrecomp {
void RegisterFightLifts() {
    RegisterLifts(kLifts, sizeof(kLifts) / sizeof(kLifts[0]));
}
} // namespace satrecomp
