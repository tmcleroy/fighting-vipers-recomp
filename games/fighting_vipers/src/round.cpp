// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
// Fighting Vipers: a round from its set-up to its end, rewritten from the game's code as readable C++
// (satrecomp/docs/READABLE.md; see fight.cpp for what the lines marked "left" are).
//
// A fight is a list of states (fv.h, kFightState); the game calls the routine of the state it is in once a
// frame (from 0x0601C4C4), and a routine moves the fight on by setting the next state. Several of the
// game's routines run on into the next state's when they are done; so do these.
#include "fv.h"
#include "game.h"
#include "lift.h"

namespace {

using satrecomp::Guest;
using namespace fv;

// The feature "Quick rounds" (cheats.cpp): the waits before and after a round are cut short. A wait is a
// count in kStartCountdown that the game takes down; this takes it down to `frames` at once.
void QuickWait(Guest &g, int32_t frames) {
    if (satrecomp::FeatureOn("quick_rounds") && static_cast<int32_t>(g.Read32(kStartCountdown)) > frames) {
        g.Write32(kStartCountdown, static_cast<uint32_t>(frames));
    }
}

// @replaces 0x0601CD3C (the fight's state 4)
// Before a round: has the announcement shown until the countdown is over, then lets the fight begin.
void RoundCountdown(Guest &g) {
    if (static_cast<int32_t>(g.Read32(kStartCountdown)) < 190) { // (the first frames do things the fighters need)
        QuickWait(g, 40);
    }
    const int32_t countdown = static_cast<int32_t>(g.Read32(kStartCountdown));
    g.R(11) = static_cast<uint32_t>(countdown); // left (the game's routine does not put r11 and r12 back)
    g.R(12) = kStartCountdown;
    g.Call(0x0601E2B8); // (shows the announcement and counts down)
    if (countdown <= 0) {
        if (g.Read8(kRoundEvents) & kRoundSuddenDeath) {
            ClearBits(g, kFightSwitches2, 0x01);
        }
        g.Write32(kFightState, state::kBegin);
    }
}

// @replaces 0x0601CAA4 (the fight's state 3)
// Sets a round up: counts it, starts the clock at the time set (half of it in sudden death), starts the
// tasks of a fight, gives each fighter their gauge, has the round announced and goes on to the countdown.
bool RoundSetUp(Guest &g) {
    const int8_t mode = static_cast<int8_t>(g.Read8(kGameMode));
    const int8_t kind = static_cast<int8_t>(g.Read8(kPlayers));
    const bool onePlayer = kind != 0 && kind != 3 && mode != kModeTeam; // one person, against the game

    g.Call(0x06013254);
    g.Call(0x0601C5D8);

    // The round's number; the word at 0x060E003E turns a quarter less for each round fought
    int32_t roundsFought = static_cast<int8_t>(g.Read8(kRound));
    if (mode == kModePlayback) {
        roundsFought--;
    }
    g.Write16(0x060E003E, static_cast<uint16_t>(-(roundsFought * 0x4000)));
    g.Write8(kRound, static_cast<uint8_t>(roundsFought + 1));

    g.Write8(0x06085B7A, 0);
    g.Call(0x0602438C);
    StartTask(g, task::kClock, 0x06024CD4); // (which goes on as RoundClock)
    g.Call(0x060243A0);
    g.Write8(0x060E000F, 0);

    uint32_t flags = g.Read32(kRoundFlags) & ~kRoundTimeUp;
    if (static_cast<int8_t>(g.Read8(kModeTable + static_cast<uint32_t>(mode * 4) + 1)) > 0 && onePlayer && g.Read8(0x060E0027) != 3) {
        flags |= kRoundJoinIn;
    }
    g.Write32(kRoundFlags, flags);
    g.Write8(0x060E000C, 0);
    g.Write8(0x060E000D, 0);
    g.Write8(0x060E0011, 0);
    g.Write8(0x060E0035, g.Read8(0x002000E6));

    // The clock
    const bool suddenDeath = flags & kRoundSuddenDeath;
    const uint32_t time = g.Read32(kRoundTimeSetting);
    g.Write32(kRoundTime, suddenDeath ? time / 2 : time);
    g.Call(kDrawRoundTime);

    // The tasks of a fight
    g.Write32(0x06062754, 0);
    g.Write32(0x06062794, 1);
    g.Call(0x06019448);
    if (mode != kModePlayback) {
        g.Call(0x060163F4);
        StartTask(g, 0x060859FC, 0x06016454);
    }
    StartTask(g, task::kFighters, 0x0600FE80);
    g.Write32(0x060E0098, 0);
    StartTask(g, 0x06085900, 0x06017B74);
    StartTask(g, 0x06085820, 0x06028234);
    StartTask(g, 0x060859E0, 0x06004B7C);
    g.Call(0x06012F78);
    StartTask(g, task::kFighter2, 0);
    StartTask(g, 0x06085AC0, 0x06018494);

    SetBits(g, kFighter1 + 1, 0x80);
    SetBits(g, kFighter2 + 1, 0x80);
    if (mode == kModePlayback) {
        g.Write32(kFighter1 + 0x5C, g.Read32(0x060600D4 + 0x4C));
        g.Write32(kFighter2 + 0x5C, g.Read32(0x060600D4 + 0x50));
        g.R(4) = 0;
        g.Call(0x0600B124);
        g.R(4) = 1;
        g.Call(0x0600B124);
    }

    // The gauges. Each player's is the one set for them: a long one where that is switched on, and in sudden
    // death a single point (unless it is long). Where the winner stays on, they keep what they had left.
    int32_t gauge[2];
    for (int player = 0; player < 2; player++) {
        const uint32_t setting = kGaugeSetting + static_cast<uint32_t>(player) * 2;
        const uint32_t fighter = player ? kFighter2 : kFighter1;
        gauge[player] = static_cast<int16_t>(g.Read16(fighter + fighter::kGauge));
        if (g.Read8(kLongGauge + static_cast<uint32_t>(player)) != 0) {
            gauge[player] = kGaugeLong;
            g.Write16(setting, kGaugeLong);
        }
    }
    const uint8_t lastWinner = g.Read8(kLastWinner);
    for (int player = 0; player < 2; player++) {
        const uint32_t setting = kGaugeSetting + static_cast<uint32_t>(player) * 2;
        if (suddenDeath) {
            if (gauge[player] != kGaugeLong) {
                gauge[player] = 1;
                g.Write16(setting, 1);
            }
        } else {
            const bool staysOn = mode == kModeTeam && lastWinner != 0 && g.Read8(0x06090205) != 0 && (lastWinner == 1) == (player == 0);
            if (!staysOn) {
                gauge[player] = static_cast<int16_t>(g.Read16(setting));
            }
        }
    }
    g.Write16(kFighter1 + fighter::kGauge, static_cast<uint16_t>(gauge[0]));
    g.Write16(kFighter2 + fighter::kGauge, static_cast<uint16_t>(gauge[1]));
    g.R(6) = kFighter2 + fighter::kGauge; // left

    g.Write32(0x060E00AC, g.Read32(0x0607E86C + static_cast<uint32_t>(static_cast<int8_t>(g.Read8(kStage)) * 4)));

    ClearBits(g, kFightSwitches3, 0x10);
    SetBits(g, kFightSwitches2, 0x01);
    SetBits(g, kFightSwitches3, 0x08 | 0x20);
    if (mode == kModePlayback) {
        ClearBits(g, kFightSwitches3, 0x80 | 0x08);
    }
    g.Write8(0x06085B86, 0);
    if (onePlayer) {
        SetBits(g, kFightSwitches3, 0x80);
    } else {
        ClearBits(g, kFightSwitches3, 0x80 | 0x08);
    }

    // The announcement ("Round 1"...; none in sudden death), and the countdown: longer before the first
    const int8_t round = static_cast<int8_t>(g.Read8(kRound));
    if (!suddenDeath) {
        g.R(4) = g.Read32(0x0607DAD8 + static_cast<uint32_t>(round * 4));
        g.Call(0x060131A8);
    }
    g.Write32(kStartCountdown, round == 1 ? 192 : 96);
    g.Call(0x0601338C);
    g.Write32(kFightState, state::kCountdown);

    RoundCountdown(g);
    g.Return(400);
    return true;
}

// @replaces 0x0601CEAA (state 6)
// Waits for the countdown, then has "Fight!" called.
void BeginWait(Guest &g) {
    if (static_cast<int32_t>(g.Read32(kStartCountdown)) > 0) {
        return;
    }
    g.Write32(kFightState, state::kLetGo);
    ClearBits(g, kFightSwitches2, 0x01);
    g.R(4) = 0xA01132FF;
    g.Call(kAnnounce);
}

// @replaces 0x0601CE7C (state 5)
// The round is about to begin: puts the picture for it up (another one if the byte at 0x060E002C is set)
// and waits 42 frames.
bool Begin(Guest &g) {
    g.Write8(0x0605FBD4 + 0x54, g.Read8(0x060E002C) != 0 ? 22 : 11);
    g.Write32(kStartCountdown, 42);
    QuickWait(g, 20);
    SetBits(g, kFightSwitches2, 0x02);
    g.Write32(kFightState, state::kBeginWait);
    BeginWait(g);
    g.Return(80);
    return true;
}

// @replaces 0x0601D014 (state 8)
// The fight itself, as far as the round goes: it is over when somebody is knocked out or the clock has run
// out (RoundClock says so). Then the clock and the fight's tasks stop.
void Fighting(Guest &g) {
    if (!(g.Read32(kRoundFlags) & kRoundTimeUp)) {
        uint8_t down = 0;
        if (g.Read8(kFighter1 + fighter::kStatus) & fighter::kKnockedOut) {
            down |= 1;
        }
        if (g.Read8(kFighter2 + fighter::kStatus) & fighter::kKnockedOut) {
            down |= 2;
        }
        g.Write8(kDown, down);
        if (!down) {
            return;
        }
    }
    ClearBits(g, kRoundEvents, kRoundJoinIn);
    StopTask(g, task::kClock);
    StopTask(g, task::kDuringFightA);
    StopTask(g, task::kDuringFightB);
    ClearBits(g, kFightSwitches + 1, 0x02);
    ClearBits(g, kFightSwitches3, 0x02);
    g.Write32(kFightState, state::kRoundOver);
}

// @replaces 0x0601CEEC (state 7)
// Lets the fighters go: they are no longer held, the clock runs, the fight's tasks start.
bool LetGo(Guest &g) {
    const int8_t mode = Byte(g, kGameMode);
    ClearBits(g, kFighter1 + 1, 0x80);
    ClearBits(g, kFighter2 + 1, 0x80);
    g.Write32(0x06066E54 + 12, g.Read32(0x06066E54 + 12) | 0x04 | 0x08);
    g.Write32(kFightSwitches, (g.Read32(kFightSwitches) & ~0x200u) | 0x20000 | 0x10 | 0x02);
    if (mode == kModePlayback || Byte(g, kRound) == 1) {
        g.R(4) = static_cast<uint32_t>(static_cast<int32_t>(Byte(g, kStage)));
        g.Call(0x06029A30);
    }
    g.Write32(0x06062754, g.Read32(0x06062754) | 3);
    g.Write32(0x06062794, g.Read32(0x06062794) | 3);
    StartTask(g, 0x060858AC, 0x06014BF0);
    StartTask(g, 0x060858C8, 0);
    g.Write32(kFightState, state::kFighting);
    SetBits(g, kClockSwitches, kClockCountsDown | kClockCountsTimeTaken);
    StartTask(g, task::kDuringFightA, 0x0601E39C);
    StartTask(g, task::kDuringFightB, 0x0601E404);
    g.Write8(0x060E0003, g.Read8(kPlayers));
    if (mode == kModePlayback) {
        ClearBits(g, kFightSwitches3, 0x02 | 0x10);
        ClearBits(g, kFightSwitches + 1, 0x02);
        SetBits(g, kFightSwitches + 1, 0x08);
        StartTask(g, 0x06085A18, 0x060165B4);
        StopTask(g, task::kDuringFightA);
        StopTask(g, task::kDuringFightB);
    }
    if (mode == kModeTeam) {
        ClearBits(g, kFightSwitches3, 0x02 | 0x10);
        ClearBits(g, kFightSwitches + 1, 0x02);
        SetBits(g, kFightSwitches + 1, 0x10);
    }
    Fighting(g);
    g.Return(200);
    return true;
}

// @replaces 0x0601D436 (state 10)
// Waits for the round's end to have been shown, then goes on to what comes after a round.
void RoundOverWait(Guest &g) {
    QuickWait(g, 45);
    if (static_cast<int32_t>(g.Read32(kStartCountdown)) > 0) {
        return;
    }
    ClearBits(g, kFightSwitches3, 0x10);
    ClearBits(g, kFightSwitches2, 0x40);
    g.Write32(kFightState, Byte(g, kGameMode) == kModePlayback ? state::kAfterRoundPlayback : state::kAfterRound);
}

// @replaces 0x06007E08 (r4: 0 player 1, 1 player 2)
int32_t Gauge(Guest &g, int player) {
    return static_cast<int16_t>(g.Read16((player > 0 ? kFighter2 : kFighter1) + fighter::kGauge));
}
// @replaces 0x06007DEC
int32_t GaugeSet(Guest &g, int player) {
    return static_cast<int16_t>(g.Read16(kGaugeSetting + (player > 0 ? 2 : 0)));
}

// @replaces 0x0602F680
// Who wins when the clock runs out: whoever has the larger part of their gauge left (1 or 2; 0 if both
// have the same). Left in the word at kTimeUpWinner.
int JudgeTimeUp(Guest &g) {
    const uint32_t part1 = static_cast<uint32_t>(Gauge(g, 0)) * static_cast<uint16_t>(GaugeSet(g, 1));
    const uint32_t part2 = static_cast<uint32_t>(Gauge(g, 1)) * static_cast<uint16_t>(GaugeSet(g, 0));
    const int winner = part1 == part2 ? 0 : part1 > part2 ? 1 : 2;
    g.Write16(kTimeUpWinner, static_cast<uint16_t>(winner));
    return winner;
}

// Who won the round: 1 or 2, or 0 for a draw
int RoundWinner(Guest &g, bool timeUp) {
    if (timeUp) {
        return JudgeTimeUp(g);
    }
    switch (Byte(g, kDown)) {
    case 1: return 2;
    case 2: return 1;
    case 3: return 0;
    default:
        // (nobody is down and the clock has not run out: the game's code has this, for a case that should not
        // happen; what the two routines do was not looked at)
        g.R(4) = 14;
        g.R(5) = 10;
        g.Call(0x0600C1A4);
        g.R(5) = g.R(0);
        g.R(4) = 0x0605F1D0;
        g.Call(0x0600C228);
        return 0;
    }
}

// A match of team play is over: the loser's team sends its next member, or has none left
void TeamMatchOver(Guest &g) {
    ClearBits(g, kFightSwitches + 1, 0x10);
    int32_t member1 = Byte(g, 0x060957E3), member2 = Byte(g, 0x060957E1); // (how many of each team have lost)
    const int32_t size1 = Byte(g, 0x060957E2), size2 = Byte(g, 0x0609580A);
    const uint32_t resultAt = 0x060957F0 + static_cast<uint32_t>(member2 * 5 + member1);
    uint8_t winner, teamOut = 0;
    if (Byte(g, kRoundWinner) != 2) {
        winner = 1;
        g.Write8(0x060957CC, g.Read8(0x060957CC) + 1);
        g.Write8(resultAt, winner);
        if (++member2 >= size2) {
            teamOut = 1;
        }
    } else {
        winner = 2;
        g.Write8(0x060957A1, g.Read8(0x060957A1) + 1);
        g.Write8(resultAt, winner);
        if (++member1 >= size1) {
            teamOut = 2;
        }
    }
    g.Write8(kLastWinner, winner);
    g.Write8(0x060957E0, teamOut);
    g.Write8(0x060957E3, static_cast<uint8_t>(member1));
    g.Write8(0x060957E1, static_cast<uint8_t>(member2));
    g.R(6) = teamOut; // left
}

// Somebody has the rounds a match takes
void MatchOver(Guest &g) {
    const int8_t mode = Byte(g, kGameMode);
    const int8_t winner = Byte(g, kRoundWinner);
    SetBits(g, kRoundEvents, kRoundMatchOver);
    // (in low work RAM, what is kept with the settings: a mark when the one player wins the fight for which
    // the byte at 0x060E0027 is 3; the number of matches played, and a mark from the fiftieth)
    if (Byte(g, 0x060E0027) == 3 && winner == Byte(g, kPlayers)) {
        SetBits(g, 0x00200080, 0x02);
        g.Write8(0x060E0027, 4);
    }
    const uint32_t matches = g.Read32(0x00200048) + 1;
    g.Write32(0x00200048, matches);
    if (static_cast<int32_t>(matches) >= 50) {
        SetBits(g, 0x00200080, 0x04);
    }
    // The records of versus play: how often each character was played, and who beat whom how often
    const uint32_t charactersAt = 0x06091EE0;
    if (mode == kModeVersus) {
        for (uint32_t player = 0; player < 2; player++) {
            const uint32_t playedAt = 0x00200000 + static_cast<uint32_t>(Byte(g, charactersAt + player) * 4);
            g.Write32(playedAt, g.Read32(playedAt) + 1);
        }
    }
    if (mode == kModeTeam) {
        TeamMatchOver(g);
    }
    if (mode == kModeVersus) {
        const int32_t character1 = Byte(g, charactersAt), character2 = Byte(g, charactersAt + 1);
        const bool first = winner == 1;
        const int32_t won = first ? character1 : character2, lost = first ? character2 : character1;
        const uint32_t countAt = (first ? 0x00200200 : 0x00200400) + static_cast<uint32_t>(32 * (won & 0xFFFF) + lost * 2);
        g.Write16(countAt, g.Read16(countAt) + 1);
    }
}

// @replaces 0x0601D0F0 (state 9)
// The round is over. Works out who won it (on time: kJudgeTimeUp; a draw of the deciding round goes to
// player 2 if player 1 plays alone against the game), counts the rounds won, sees whether that is the
// match, and has the result called and shown.
bool RoundOver(Guest &g) {
    g.Write32(0x06066E54 + 12, (g.Read32(0x06066E54 + 12) | 0x20) & ~0x04u);
    g.Write32(kStartCountdown, 96);
    g.R(11) = kFighter1; // left (the game's routine does not put r8, r11 and r12 back)
    g.R(12) = kFighter2;

    const bool timeUp = g.Read8(kRoundEvents) & kRoundTimeUp;
    g.Write8(kRoundEndedBy, timeUp ? 2 : 0);
    int winner = RoundWinner(g, timeUp);
    if (winner == 0 && (g.Read8(kRoundEvents) & kRoundSuddenDeath)) {
        // The deciding round must have a winner
        const int8_t players = Byte(g, kPlayers), byE = Byte(g, 0x060E000E);
        if (players == 3 && byE == 2) {
            winner = 2;
        } else if (players == 3 && byE == 1) {
            winner = 1;
        } else {
            winner = players == 1 ? 2 : 1;
        }
    }

    const int32_t needed = Byte(g, kWinsNeeded);
    bool matchOver;
    if (winner == 0) {
        // A draw: a round to each. That is the match only if one of them now has the rounds and the other not.
        g.Write8(kRoundResult, 3);
        g.Write8(kWins, g.Read8(kWins) + 1);
        g.Write8(kWins + 1, g.Read8(kWins + 1) + 1);
        const int32_t wins1 = Byte(g, kWins), wins2 = Byte(g, kWins + 1);
        matchOver = false;
        if (wins1 != wins2) {
            g.Write8(kRoundWinner, wins1 >= needed ? 1 : wins2 >= needed ? 2 : 3);
            matchOver = wins1 >= needed || wins2 >= needed;
        }
    } else {
        const uint32_t loserBit = winner == 1 ? 0x02 : 0x01;
        g.Write32(0x06062754, g.Read32(0x06062754) & ~loserBit);
        g.Write32(0x06062794, g.Read32(0x06062794) & ~loserBit);
        g.Write32(0x060600E0, winner == 1 ? kFighter1 : kFighter2); // the winner's record, and the loser's
        g.Write32(0x060600E4, winner == 1 ? kFighter2 : kFighter1);
        g.Write8(kRoundResult, static_cast<uint8_t>(winner));
        g.Write8(kRoundWinner, static_cast<uint8_t>(winner));
        const uint32_t winsAt = kWins + static_cast<uint32_t>(winner - 1);
        g.Write8(winsAt, g.Read8(winsAt) + 1);
        matchOver = Byte(g, winsAt) >= needed;
    }
    if (matchOver) {
        MatchOver(g);
    }

    // What is called and shown
    enum : uint8_t { kShowKnockOut = 3, kShowDraw = 4, kShowTimeOver = 5, kShowPerfect = 6 };
    uint8_t show;
    if (Byte(g, kRoundResult) == 3) {
        g.R(4) = 0xA01131FF;
        g.Call(kAnnounce);
        show = kShowDraw;
    } else if (Byte(g, kRoundEndedBy) == 1) {
        show = kShowPerfect;
    } else if (Byte(g, kRoundEndedBy) == 2) {
        g.R(4) = 0xA01135FF;
        g.Call(kAnnounce);
        show = kShowTimeOver;
    } else {
        g.Call(0x0601A3F4);
        // (a knockout by somebody whose gauge is still whole)
        const bool whole1 = static_cast<int16_t>(g.Read16(kFighter1 + fighter::kGauge)) >= static_cast<int16_t>(g.Read16(kGaugeSetting));
        const bool whole2 = static_cast<int16_t>(g.Read16(kFighter2 + fighter::kGauge)) >= static_cast<int16_t>(g.Read16(kGaugeSetting + 2));
        show = whole1 || whole2 ? kShowPerfect : kShowKnockOut;
    }
    g.R(8) = show; // left
    SetBits(g, kFightSwitches2, 0x40);
    g.Write8(0x06066CAE, show);
    g.Write32(kFightState, state::kRoundOverWait);

    RoundOverWait(g);
    g.Return(300);
    return true;
}

// @replaces 0x0601C55C
// May the other player come into the fight, and have they pressed Start? Then the fight goes to the state
// that takes them in. (Also when the byte at 0x060E0027 is 2, whatever sets that.)
bool JoinIn(Guest &g) {
    if ((g.Read8(kFightPaused) & kFightPausedBit) || !(g.Read8(kRoundEvents) & kRoundJoinIn)) {
        return false;
    }
    if (Byte(g, 0x060E0027) == 2) {
        g.Write8(0x060E0028, 1);
        g.Write8(0x060E0027, 3);
    } else {
        g.R(7) = kPads; // left
        const int8_t players = Byte(g, kPlayers);
        const bool start1 = g.Read8(kPads + pad::kPressed) & pad::kStartHigh, start2 = g.Read8(kPads + pad::kSize + pad::kPressed) & pad::kStartHigh;
        if (!(start1 && players == 2) && !(start2 && players == 1)) {
            return false;
        }
    }
    g.Write32(kFightState, state::kJoinIn);
    return true;
}

bool JoinInAlone(Guest &g) {
    g.R(0) = JoinIn(g);
    g.Return(40);
    return true;
}

// @replaces 0x0601C4C4 (the game's main state while a fight is on)
// A frame of the fight: unless somebody is coming in, what goes with the state it is in (the byte beside
// its routine in the table; 0x060226D6 takes it) and, unless the fight is paused, the state's routine.
bool FightStep(Guest &g) {
    // (Only for the states of a round, whose routines are this file's. Those of other states wait for the disc
    // and the like, and a routine called from here must come back at once: the game's own code runs those.)
    const uint32_t state = g.Read32(kFightState);
    if (state < state::kSetUp || state > state::kRoundOverWait) {
        if (state == state::kWinner) {
            QuickWait(g, 60);
        }
        return false;
    }
    const uint32_t entry = kFightStates + state * 8;
    g.R(14) = entry; // left (the game's routine does not put r14 back)
    if (!JoinIn(g)) {
        g.R(4) = static_cast<uint32_t>(static_cast<int32_t>(Byte(g, entry + 4)));
        g.Call(0x060226D6);
    }
    const uint32_t flags = g.Read32(kFightFlags);
    g.R(4) = flags; // left
    if (!(flags & kFightPausedBit)) {
        g.Call(g.Read32(entry));
        g.Call(0x060132E4);
    }
    g.Return(60);
    return true;
}

template <void (*Routine)(Guest &)>
bool Alone(Guest &g) {
    Routine(g);
    g.Return(60);
    return true;
}

bool GaugeAlone(Guest &g) {
    g.R(0) = static_cast<uint32_t>(Gauge(g, static_cast<int32_t>(g.R(4))));
    g.Return(12);
    return true;
}
bool GaugeSetAlone(Guest &g) {
    g.R(0) = g.R(5) = static_cast<uint32_t>(GaugeSet(g, static_cast<int32_t>(g.R(4))));
    g.Return(12);
    return true;
}
bool JudgeTimeUpAlone(Guest &g) {
    JudgeTimeUp(g);
    g.Return(120);
    return true;
}

// (free: what the callers of the fight's states, 0x0601C4C4 and 0x0601C514, do not read after them)
constexpr uint32_t kFree = 0x160BF;
const satrecomp::Lift kLifts[] = {
    {"RoundSetUp", 0x0601CAA4, 0x298, 0x103929A98224F276ull, RoundSetUp, true, kFree},
    {"RoundCountdown", 0x0601CD3C, 0x34, 0x624A7CE5FEE92AE5ull, Alone<RoundCountdown>, true, kFree},
    {"Begin", 0x0601CE7C, 0x2E, 0xBBC6B0CED24FBA6Eull, Begin, true, kFree},
    {"BeginWait", 0x0601CEAA, 0x24, 0x9C9CB6316FC4596Bull, Alone<BeginWait>, true, kFree},
    {"LetGo", 0x0601CEEC, 0x128, 0xFE4C300044816387ull, LetGo, true, kFree},
    {"Fighting", 0x0601D014, 0x6E, 0xD2A00AE80762A4C8ull, Alone<Fighting>, true, kFree},
    {"RoundOver", 0x0601D0F0, 0x346, 0xC98027DC8B17009Bull, RoundOver, true, kFree},
    {"RoundOverWait", 0x0601D436, 0x32, 0x4104C7F99DAADA35ull, Alone<RoundOverWait>, true, kFree},
    // (free: from tools/liveout.py for each)
    {"FightStep", 0x0601C4C4, 0x50, 0x3570FFE6C17A020Eull, FightStep, true, 0x1000F},
    {"JoinIn", 0x0601C55C, 0x7C, 0x94E3F84D3A6FE24Bull, JoinInAlone, true, 0x1001E},

    {"JudgeTimeUp", 0x0602F680, 0x70, 0x58B49466E0F6BDBDull, JudgeTimeUpAlone, true, 0x101BF},
    {"GaugeSet", 0x06007DEC, 0x1C, 0x4D8F25B7AC1873B1ull, GaugeSetAlone, true, 0x15B1E},
    {"Gauge", 0x06007E08, 0x1C, 0x9662A2663402B16Eull, GaugeAlone, true, 0x10016},
};

} // namespace

namespace satrecomp {
void RegisterRoundLifts() {
    RegisterLifts(kLifts, sizeof(kLifts) / sizeof(kLifts[0]));
}
} // namespace satrecomp
