// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
// Fighting Vipers: training aids (optional features in the category "Training"; the launcher's Features page
// and the in-game menu switch them).
//
//   Damage readout   by each gauge, what the last hit took off it, and for a combo the number of hits and
//                    what they took together. (A combo: hits between which the fighter hit could not
//                    act, by the busy bit of its state.)
//   Gauges refill    a gauge is whole again a second and a half after its fighter was last hit.
//   Input display    down each side, each fighter's last inputs, the newest at the top, with the frames
//                    each was held, in the notation fighting games are written about in: the direction as the digit at its place on a number pad
//                    for a fighter facing right (6 forward, 4 back, 2 down, 8 up, 3 down and forward...),
//                    then the buttons (G guard, P punch, K kick).
//   Frame data       after a move hits or is guarded, by the gauge of whoever did it: the frame it hit on,
//                    and how many frames sooner (+) or later (-) than the other they can act again.
//                    A throw, the same way: the frames from the buttons to the grab, the frame of being
//                    thrown on which the damage is taken, and how far ahead the thrower is at the end.
//   Move data        by the gauge of whoever did it, the last attack's row of the table below: its level
//                    (high, middle, low), the frame it first hits on, the frames it can hit for, the frames
//                    after that until its fighter can act; its damage to the gauge and to the armour; how
//                    far ahead it leaves its fighter on hit and on guard, once each has been seen. Every
//                    attack done is kept in a table by character and move, and the table is written to a
//                    file (moves-<character>.csv, beside the settings) to be read against published ones.
//   Move log         every attack done, a line of frame_data.csv beside the settings, once what came of it
//                    is known: what was pressed for it, its frames, the result, the advantage, the damage.
//   Throw range      on the ground before each fighter, the reach of its throw (a wedge of dots: the
//                    game's own distance and angle), lit while the game would let the throw happen; and a
//                    line with the distance, the reach, and what is in the way. For a fighter being
//                    thrown the line is the presses counted so far, of those it takes to break free.
//   Hit overlay      the same as the hit view, laid over the fighters by the game's view.
//   Hit view         a diagram of what the game tests for hits: each fighter's body as the game has it, 22
//                    balls, seen from the side. Outlined: parts that can be hit (grey: cannot, now); red:
//                    parts the fighter's move attacks with; yellow: parts being touched by an attack.
//   Armour readout   by each gauge, what is left of the upper and the lower armour (its strength less the
//                    damage it has taken), or OFF.
//   Armour           (category "Armour", one at a time) the armour never comes off (and is put back if it
//                    has); or the opponent has none; or nobody has.
//   Dummy            (category "Dummy", one at a time) the opponent stands, crouches, guards (standing,
//                    low, standing or low by chance for each attack, or only after a hit; or guards and
//                    counters as soon as it can, with the move chosen under "Dummy counter") or jumps
//                    instead of fighting; or is played by the player's own pad and recorded (up to ten
//                    seconds, while the player's own fighter stands); or plays the recording over and over.
//
//   Dummy recovery   (one at a time; with any dummy, or with the game's own play) what the opponent does
//                    when it is knocked down: lands on its feet, gets up fast, rolls back, forward or
//                    aside, or gets up with a kick or a low kick; or one of those or nothing, by chance as it
//                    goes down. With none on it lies there as long as the game lets it.
//   Dummy wake-up    (one at a time; the same) what the opponent does the moment it can act again after
//                    being down: guards, punches, kicks, kicks low, throws, or does what was recorded; or one
//                    of those or nothing, by chance as it gets up.
//   Dummy answers    now and then: its counter and its wake-up only one time in two, by chance.
//   Dummy struggles  (always, or in a throw in two by chance as the throw begins) thrown, the opponent
//                    presses the buttons all the while. This is the game's way out of
//                    a throw: it counts the presses of whoever is thrown, and with enough of them by the
//                    end of the first part of being thrown (more than 18 in 52 frames, for the throw of
//                    guard and punch) the fighter breaks free and takes no damage.
//
// All of these but Armour are training aids: they work in the game's own training mode (TRAINING MODE in its
// main menu; fv.h, kModeTraining) and nowhere else.
//
// The first two hang on the rewritten rules: TakeDamage reports each hit, FightersTask each frame of the
// fight. The dummy is put in where a player's input for the frame is taken (fightinput.cpp, PlayerInput).
#include "training.h"

#include "fv.h"
#include "game.h"
#include "state.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

namespace fv::training {

namespace {
    constexpr uint32_t kShownFor = 150;   // frames the readout stays after the last hit
    constexpr uint32_t kRefillAfter = 90; // frames after the last hit when the gauge is made whole
    constexpr uint32_t kLongAgo = 100000;

    struct Hits {
        int32_t last = 0, together = 0; // the last hit's damage, and that of the row it is in
        uint32_t count = 0;             // hits in the row
        uint32_t since = kLongAgo;      // frames since the last hit
        bool couldAct = true;           // since the last hit, the fighter has had a frame on which it could act
    };
    constexpr uint32_t kTapeFrames = 600;
    constexpr uint32_t kInputsShown = 10;

    // An attack (or a throw) as it was seen done: what the move's data says of it, and what was counted
    // from the fighter's state while it went on. Frames are counted from 1, as the game counts a move's.
    struct Attack {
        enum : uint8_t { kNothing, kHit, kGuarded, kKnockedDown, kThrew };
        bool going = false, throws = false;
        uint8_t character = 0, level = 0, power = 0;
        uint8_t input = 0;                 // what its fighter pressed for it: the input when a button was last pressed (right is forward)
        uint8_t result = kNothing;         // what came of it; for kHit and kGuarded `advantage` holds
        int16_t advantage = 0;
        uint16_t move = 0, lastFrame = 0;  // (lastFrame: the move's frame when last looked at)
        uint16_t hitsFrom = 0, hitsUntil = 0, freeFrom = 0, length = 0; // the move's data (fv.h)
        uint16_t frames = 0;               // counted so far
        uint16_t startUp = 0;              // frames before it could hit
        uint16_t lastActive = 0;           // the last frame on which it could
        uint16_t total = 0;                // frames until its fighter could act again (0: not got to, or cut short)
        int16_t damage = 0, armour = 0;    // taken off the other's gauge, added to the damage of its armour
        uint32_t serial = 0;               // (which attack of the fight this is)
        uint32_t since = kLongAgo;         // frames since it began
    };

    struct State {
        Hits hits[2];           // taken by each player
        uint32_t fightSeen = 0; // (counts down after the last frame of a fight: the readout goes with it)
        uint32_t tapeAt = 0;
        bool recording = false;
        uint8_t playersInput = 0; // (what the player's pad said this frame, for the dummy while recording)
        float view[12] = {};      // the world to the view, as the fighters were last drawn
        bool viewSeen = false;
        // Frame data: each fighter's part in the exchange going on, and the last one's result
        struct Exchange {
            bool busy = false, reacted = false, guarded = false; // (reacted: was hit, guarded or was thrown while busy)
            bool thrown = false;
            uint32_t busyFor = 0, freeFor = kLongAgo;            // frames busy; frames since no longer busy
            uint32_t by = 0;                                     // (the serial of the other's attack it reacted to)
        } exchange[2];
        uint32_t hitOnFrame = 0; // the frame of the attacker's move on which it hit or was guarded
        uint32_t grabbedAfter = 0; // a throw: frames from the thrower's last press of a button to the grab
        struct Result {
            int32_t advantage = 0;
            uint32_t hitOnFrame = 0, shown = kLongAgo;
            bool guarded = false;
            int attacker = 0;
            bool thrown = false;
            uint32_t grabbedAfter = 0;
        } result;
        Attack attack[2];          // each fighter's attack going on, or its last
        uint32_t attacks = 0;      // (counted, for Attack::serial)
        uint32_t armourSeen[2] = {}; // the damage each fighter's armour had when last looked at (both parts)
        uint16_t sincePress[2] = {}; // frames since each fighter's input last had a button newly pressed
        uint8_t pressed[2] = {};     // ... and that input
        // The dummy's reply (its counter, or what it does as it gets up) being pressed, and what leads to one
        uint8_t reply = 0;         // (Reply)
        uint32_t replyAt = 0, replyFor = 0; // the frame of it, and how many it takes
        bool wasDown = false;      // the dummy has been down and has not been able to act since
        bool counterDue = false;   // the dummy was hit or guarded and has not been able to act since
        bool guardsLow = false;    // the dummy that guards by chance: how it guards the attack going on
        // The dummy at random: the way up chosen for the knock-down going on (0: none; RecoveryWay), the
        // throw going on struggled out of or not; and whether it answers the exchange going on ("Answers now
        // and then"), for the attack with that serial
        uint8_t recoveryPick = 0;
        bool downPicked = false, thrownSeen = false, struggles = false;
        uint32_t answerFor = 0;
        bool answers = true;
        uint32_t logged[2] = {}; // the move log: the serial of each fighter's attack last written
        uint32_t chance = 0x2F6E2B1Du; // (the state of what makes the chances)
        // The input display: each fighter's last inputs (right is forward), the newest last
        struct Inputs {
            uint8_t shown[kInputsShown] = {};
            uint16_t frames[kInputsShown] = {}; // how long each was held
            uint32_t count = 0;
        } inputs[2];
    } g_state;

    // The dummy's recording: its input a frame, left and right as towards and away from the other. (Not part
    // of g_state, which goes into save states: going back to the reset point must not take the recording
    // back with it.)
    struct Tape {
        uint8_t input[kTapeFrames] = {};
        uint32_t taped = 0;
    } g_tape;
    // (FV_TAPE=hex bytes, for tests: a recording to start with, a byte of input a frame)
    const bool g_tapeGiven = [] {
        const char *given = getenv("FV_TAPE");
        for (; given && given[0] && given[1] && g_tape.taped < kTapeFrames; given += 2) {
            const char byte[3] = {given[0], given[1], 0};
            g_tape.input[g_tape.taped++] = static_cast<uint8_t>(strtoul(byte, nullptr, 16));
        }
        return given != nullptr;
    }();

    // Which player the dummy is: the opponent of whoever plays (player 2, when both do, or neither).
    // (FV_DUMMY_PLAYER=0 or 1, for tests: that one.)
    int DummyPlayer(uint8_t players) {
        static const char *forced = getenv("FV_DUMMY_PLAYER");
        return forced ? atoi(forced) & 1 : players == 2 ? 0 : 1;
    }

    // The moves seen done, by character and move: the table "Move data" shows a row of and writes to a
    // file. (Not part of g_state, as the tape is not: going back to the reset point loses nothing learnt.)
    struct MoveRow {
        Attack attack;                    // as last seen done to its end (or, until one was, as far as seen)
        bool hitKnown = false, guardKnown = false, knocksDown = false;
        int hit = 0, guard = 0;           // how far ahead it leaves its fighter
        int damage = 0, armour = 0;       // the most seen
        uint32_t times = 0;
    };
    std::map<uint32_t, MoveRow> g_moves;  // (character * 0x10000 + move)
    bool g_movesChanged = false;
    uint32_t g_movesChangedFor = 0;       // frames since the table last changed

    // The characters whose numbers were seen (fighter::kCharacter); the others go by number
    std::string CharacterName(unsigned character) {
        static const char *const kNames[] = {"grace", "bahn"};
        return character < 2 ? kNames[character] : "character-" + std::to_string(character);
    }

    const char *LevelName(uint8_t level) {
        switch (level) {
        case level::kHigh: return "HIGH";
        case level::kMiddle: return "MID";
        case level::kLow: case 5: case 6: return "LOW";
        case level::kMiddleHeavy: return "MID+";
        case level::kGround: return "GROUND";
        default: return "?";
        }
    }

    // An attack has got further, or more is known of what came of it: its row of the table
    void Tell(const Attack &attack) {
        if (satrecomp::RunningAhead()) {
            return; // (frames that will be taken back: the table is not part of save states)
        }
        MoveRow &row = g_moves[attack.character * 0x10000u + attack.move];
        if (attack.total || !row.attack.total) {
            row.attack = attack;
        }
        row.damage = std::max(row.damage, static_cast<int>(attack.damage));
        row.armour = std::max(row.armour, static_cast<int>(attack.armour));
        if (attack.result == Attack::kHit) {
            row.hitKnown = true;
            row.hit = attack.advantage;
        } else if (attack.result == Attack::kGuarded) {
            row.guardKnown = true;
            row.guard = attack.advantage;
        } else if (attack.result == Attack::kKnockedDown) {
            row.knocksDown = true;
        }
        g_movesChanged = true;
        g_movesChangedFor = 0;
    }

    // The table, a file for each character: comma-separated, a line a move. The frames are counted as the
    // readout shows them: `start` the frame it first hits on, `active` the frames it can hit for,
    // `recovery` the frames after those until the fighter can act (empty: never seen to its end).
    // (FV_MOVES_DIR=folder, for tests: there, and not beside the settings.)
    void WriteMoves() {
        g_movesChanged = false;
        static const char *folder = getenv("FV_MOVES_DIR");
        std::map<unsigned, std::string> files;
        for (const auto &[key, row] : g_moves) {
            const Attack &a = row.attack;
            std::string &text = files[key >> 16];
            if (text.empty()) {
                text = "character,move,kind,level,power,hits_from,hits_until,free_from,length,start,active,recovery,damage,armour_damage,on_hit,on_guard,knocks_down,times_seen\n";
            }
            char line[256];
            const std::string start = a.lastActive ? std::to_string(a.startUp + 1) : "", active = a.lastActive ? std::to_string(a.lastActive - a.startUp) : "",
                              recovery = a.total >= a.lastActive && a.total ? std::to_string(a.total - a.lastActive) : "";
            snprintf(line, sizeof(line), "%s,%03X,%s,%s,%u,%u,%u,%u,%u,%s,%s,%s,%d,%d,%s,%s,%s,%u\n", CharacterName(key >> 16).c_str(), a.move, a.throws ? "throw" : "attack",
                     a.throws ? "" : LevelName(a.level), a.throws ? 0u : a.power, a.hitsFrom, a.hitsUntil, a.freeFrom, a.length, start.c_str(), active.c_str(), recovery.c_str(),
                     row.damage, row.armour, row.hitKnown ? std::to_string(row.hit).c_str() : "", row.guardKnown ? std::to_string(row.guard).c_str() : "",
                     row.knocksDown ? "yes" : "", row.times);
            text += line;
        }
        for (const auto &[character, text] : files) {
            const std::string name = "moves-" + CharacterName(character) + ".csv";
            const std::string path = folder ? std::string(folder) + "/" + name : satrecomp::DataFilePath(name);
            if (path.empty()) {
                continue;
            }
            if (FILE *file = fopen(path.c_str(), "w")) {
                fwrite(text.data(), 1, text.size(), file);
                fclose(file);
            }
        }
    }

    // The fighters are fighting: a round of any mode, or the training mode's fight (which is a state of its own)
    bool Fighting(satrecomp::Guest &g) {
        const uint32_t now = g.Read32(kFightState);
        return now == state::kFighting || now == state::kTraining;
    }

    // The move log ("Move log"): every attack done, a line of frame_data.csv beside the settings, written once
    // what came of it is known: the player (1, 2), the character, what was pressed for it (the input as a button was last pressed,
    // in number pad notation as the input display has it), the move, its kind and level, the frames as the readout counts them (start, active,
    // recovery), what came of it (hit, guard, knockdown, throw, escaped; nothing: it met nothing), how far
    // ahead it left its fighter, and its damage to the gauge and to the armour. (FV_MOVES_DIR: there.)
    std::string Notation(uint8_t bits);
    void LogAttack(int player, const Attack &a) {
        static FILE *file = nullptr;
        static bool tried = false;
        if (!file) {
            if (tried) {
                return;
            }
            tried = true;
            static const char *folder = getenv("FV_MOVES_DIR");
            const std::string path = folder ? std::string(folder) + "/frame_data.csv" : satrecomp::DataFilePath("frame_data.csv");
            if (path.empty() || !(file = fopen(path.c_str(), "w"))) {
                return;
            }
            fputs("player,character,input,move,kind,level,start,active,recovery,result,advantage,damage,armour_damage\n", file);
        }
        const std::string start = a.lastActive ? std::to_string(a.startUp + 1) : "", active = a.lastActive ? std::to_string(a.lastActive - a.startUp) : "",
                          recovery = a.total >= a.lastActive && a.total ? std::to_string(a.total - a.lastActive) : "";
        const char *result = a.result == Attack::kHit           ? "hit"
                             : a.result == Attack::kGuarded     ? "guard"
                             : a.result == Attack::kKnockedDown ? "knockdown"
                             : a.result == Attack::kThrew       ? (a.damage || a.armour ? "throw" : "escaped")
                                                                : "";
        const bool told = a.result == Attack::kHit || a.result == Attack::kGuarded || a.result == Attack::kThrew;
        fprintf(file, "%d,%s,%s,%03X,%s,%s,%s,%s,%s,%s,%s,%d,%d\n", player + 1, CharacterName(a.character).c_str(), Notation(a.input).c_str(), a.move,
                a.throws ? "throw" : "attack", a.throws ? "" : LevelName(a.level), start.c_str(), active.c_str(), recovery.c_str(), result,
                told ? std::to_string(a.advantage).c_str() : "", a.damage, a.armour);
        fflush(file);
    }
    // ... once it is due: what came of it is known, or a second has gone by since it was over, or another
    // attack has taken its place
    void LogWhenDue(int player, const Attack &attack, bool replaced) {
        if (!attack.serial || attack.going || attack.serial == g_state.logged[player]) {
            return;
        }
        // (the other still reacting to it, thrown or knocked down, say: what came of it is not known yet; ten
        // seconds at most)
        const State::Exchange &other = g_state.exchange[1 - player];
        const bool reacting = other.reacted && other.by == attack.serial && attack.since < 600;
        if (!replaced && attack.result == Attack::kNothing && (reacting || attack.since < static_cast<uint32_t>(attack.frames) + 60)) {
            return;
        }
        g_state.logged[player] = attack.serial;
        // (frames run ahead will be taken back: it is written when they come again)
        if (satrecomp::FeatureOn("move_log") && !satrecomp::RunningAhead()) {
            LogAttack(player, attack);
        }
    }

    // A chance: the next of a row of numbers that is the same again from the same state
    uint32_t Chance() {
        uint32_t &x = g_state.chance;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return x;
    }

    enum class Dummy { Off, Stand, Crouch, Guard, CrouchGuard, GuardRandom, GuardAfterHit, PunchBack, Jump, Record, Replay };
    constexpr uint32_t kGuardsFor = 60; // frames the dummy that guards after a hit keeps its guard up
    Dummy DummyMode() {
        using satrecomp::FeatureOn;
        return FeatureOn("dummy_stand")    ? Dummy::Stand
               : FeatureOn("dummy_crouch") ? Dummy::Crouch
               : FeatureOn("dummy_guard")  ? Dummy::Guard
               : FeatureOn("dummy_crouch_guard") ? Dummy::CrouchGuard
               : FeatureOn("dummy_guard_random") ? Dummy::GuardRandom
               : FeatureOn("dummy_guard_after_hit") ? Dummy::GuardAfterHit
               : FeatureOn("dummy_punch_back") ? Dummy::PunchBack
               : FeatureOn("dummy_jump")   ? Dummy::Jump
               : FeatureOn("dummy_record") ? Dummy::Record
               : FeatureOn("dummy_replay") ? Dummy::Replay
                                           : Dummy::Off;
    }

    // Left and right the other way round (for a fighter on the right, so that the tape holds "towards" as
    // right)
    uint8_t Mirrored(uint8_t bits) {
        return static_cast<uint8_t>((bits & ~(input::kLeft | input::kRight)) | (bits & input::kLeft ? input::kRight : 0) | (bits & input::kRight ? input::kLeft : 0));
    }

    // What the dummy does when it is knocked down (the category "Dummy recovery"): one of these ways (1 on, in
    // this order), chance's pick of them or none for "At random", or 0 for nothing of the kind
    const char *const kRecoveryWays[] = {"recover_in_the_air", "recover_fast", "recover_back", "recover_forward",
                                         "recover_side", "recover_kick", "recover_low_kick"};
    constexpr uint32_t kRecoveryWayCount = sizeof(kRecoveryWays) / sizeof(kRecoveryWays[0]);
    int RecoveryWay() {
        for (uint32_t way = 0; way < kRecoveryWayCount; way++) {
            if (satrecomp::FeatureOn(kRecoveryWays[way])) {
                return static_cast<int>(way) + 1;
            }
        }
        return satrecomp::FeatureOn("recover_random") ? g_state.recoveryPick : 0;
    }
    // ... and the input for this frame, or -1 for nothing of the kind
    int RecoveryInput(uint32_t state, bool onTheRight, uint32_t frame) {
        if (!(state & fighter::kBusy) || !(state & (fighter::kKnockedDown | fighter::kOnTheGround))) {
            return -1;
        }
        const int way = RecoveryWay();
        const uint8_t forward = onTheRight ? input::kLeft : input::kRight, back = onTheRight ? input::kRight : input::kLeft;
        const uint8_t buttons = frame & 1 ? input::kPunch | input::kKick | input::kGuard : 0; // (pressed again and again)
        // (the buttons pressed while still in the air have the fighter land on its feet)
        if (way == 1) {
            return buttons;
        }
        if (!(state & fighter::kOnTheGround)) {
            return -1;
        }
        switch (way) {
        case 2: return buttons;
        case 3: return back;
        case 4: return forward;
        case 5: return input::kDown;
        case 6: return buttons & input::kKick;
        case 7: return input::kDown | (buttons & input::kKick);
        default: return -1;
        }
    }

    // What the dummy answers with: its counter (the category "Dummy counter") and what it does as it gets
    // up (the category "Dummy wake-up")
    enum class Reply : uint8_t { None, Punch, Kick, LowKick, Throw, Guard, Replay };
    Reply CounterChoice() {
        using satrecomp::FeatureOn;
        return FeatureOn("counter_kick")       ? Reply::Kick
               : FeatureOn("counter_low_kick") ? Reply::LowKick
               : FeatureOn("counter_throw")    ? Reply::Throw
               : FeatureOn("counter_replay")   ? Reply::Replay
                                               : Reply::Punch;
    }
    // (asked as the dummy gets up: "At random" is chance's pick, then, of the others or nothing)
    Reply WakeUpChoice() {
        using satrecomp::FeatureOn;
        if (FeatureOn("wakeup_random")) {
            static const Reply kPicks[] = {Reply::None, Reply::Guard, Reply::Punch, Reply::Kick, Reply::LowKick, Reply::Throw, Reply::Replay};
            return kPicks[Chance() % (g_tape.taped ? 7u : 6u)];
        }
        return FeatureOn("wakeup_guard")      ? Reply::Guard
               : FeatureOn("wakeup_punch")    ? Reply::Punch
               : FeatureOn("wakeup_kick")     ? Reply::Kick
               : FeatureOn("wakeup_low_kick") ? Reply::LowKick
               : FeatureOn("wakeup_throw")    ? Reply::Throw
               : FeatureOn("wakeup_replay")   ? Reply::Replay
                                              : Reply::None;
    }
    // "Answers now and then": whether the dummy answers this time, one time in two by chance. `serial` is the
    // attack it answers (the same answer for as long as that one is gone through), or 0 for its getting up.
    bool Answers(uint32_t serial) {
        if (!satrecomp::FeatureOn("dummy_answers_sometimes")) {
            return true;
        }
        if (serial == 0 || serial != g_state.answerFor) {
            g_state.answerFor = serial;
            g_state.answers = (Chance() & 0x100) != 0;
        }
        return g_state.answers;
    }
    // How long a reply is pressed for
    uint32_t ReplyFrames(Reply reply) {
        return reply == Reply::Replay ? g_tape.taped : reply == Reply::Guard ? 40 : reply == Reply::None ? 0 : 6;
    }
    // A reply's input on its nth frame: the buttons pressed again and again (every other frame, from the
    // first), or the recording from its start
    uint8_t ReplyInput(Reply reply, uint32_t n, bool onTheRight) {
        const bool press = !(n & 1);
        switch (reply) {
        case Reply::Punch: return press ? input::kPunch : 0;
        case Reply::Kick: return press ? input::kKick : 0;
        case Reply::LowKick: return input::kDown | (press ? input::kKick : 0);
        case Reply::Throw: return press ? input::kGuard | input::kPunch : 0;
        case Reply::Guard: return input::kGuard;
        case Reply::Replay: return n < g_tape.taped ? (onTheRight ? Mirrored(g_tape.input[n]) : g_tape.input[n]) : 0;
        default: return 0;
        }
    }

    int DummyInput(satrecomp::Guest &g, int player, uint8_t fromPad) {
        const Dummy mode = DummyMode();
        if (mode != Dummy::Record) {
            g_state.recording = false;
        }
        if (!g_state.fightSeen || !Fighting(g)) {
            return -1;
        }
        // The dummy is the opponent of whoever plays (player 2, when both do)
        const int dummy = DummyPlayer(g.Read8(kPlayers));
        const bool onTheRight = g.Read32((dummy ? kFighter2 : kFighter1) + fighter::kFlags) & fighter::kOnTheRight;
        // (knocked down or thrown, it does what is set for that, whatever else it is doing: also the game's
        // own play. Not while it is being recorded or replays.)
        if (player == dummy && mode != Dummy::Record && mode != Dummy::Replay) {
            const uint32_t state = g.Read32((dummy ? kFighter2 : kFighter1) + fighter::kState);
            const uint32_t frame = g_state.exchange[dummy].busyFor;
            // (thrown: struggles always, or in a throw in two as chance has it as each throw begins)
            const bool thrown = (state & fighter::kBusy) && (state & fighter::kThrown);
            if (thrown && !g_state.thrownSeen) {
                g_state.struggles = (Chance() & 0x100) != 0;
            }
            g_state.thrownSeen = thrown;
            const int struggle = satrecomp::FeatureValue("dummy_struggle");
            if (thrown && (struggle == 1 || (struggle == 2 && g_state.struggles))) {
                return frame & 1 ? input::kPunch | input::kKick | input::kGuard : 0;
            }
            // As it gets up: on the first frame it can act after having been down, its reply starts
            if ((state & fighter::kBusy) && (state & (fighter::kKnockedDown | fighter::kOnTheGround))) {
                g_state.wasDown = true;
                g_state.replyFor = 0;
                // (at random: the way up is chosen as it goes down, among the ways and none)
                if (!g_state.downPicked) {
                    g_state.downPicked = true;
                    g_state.recoveryPick = static_cast<uint8_t>(Chance() % (kRecoveryWayCount + 1));
                }
            }
            const int recovery = RecoveryInput(state, onTheRight, frame);
            if (recovery >= 0) {
                return recovery;
            }
            if (g_state.wasDown && !(state & fighter::kBusy)) {
                g_state.wasDown = false;
                g_state.downPicked = false;
                const Reply reply = Answers(0) ? WakeUpChoice() : Reply::None;
                g_state.reply = static_cast<uint8_t>(reply);
                g_state.replyAt = 0;
                g_state.replyFor = ReplyFrames(reply);
            }
            // The counter that is the recording: on the first frame it can act after being hit or guarding
            if (mode == Dummy::PunchBack && CounterChoice() == Reply::Replay) {
                const State::Exchange &own = g_state.exchange[dummy];
                if (own.busy && own.reacted) {
                    g_state.counterDue = Answers(own.by);
                } else if (g_state.counterDue && !own.busy) {
                    g_state.counterDue = false;
                    g_state.reply = static_cast<uint8_t>(Reply::Replay);
                    g_state.replyAt = 0;
                    g_state.replyFor = ReplyFrames(Reply::Replay);
                }
            } else {
                g_state.counterDue = false;
            }
            if (g_state.replyAt < g_state.replyFor) {
                return ReplyInput(static_cast<Reply>(g_state.reply), g_state.replyAt++, onTheRight);
            }
        }
        if (mode == Dummy::Off) {
            return -1;
        }
        if (player != dummy) {
            if (mode == Dummy::Record) {
                g_state.playersInput = fromPad;
                return 0;
            }
            return -1;
        }
        uint8_t bits = 0;
        switch (mode) {
        case Dummy::Crouch: bits = input::kDown; break;
        case Dummy::Guard: bits = input::kGuard; break;
        case Dummy::CrouchGuard: bits = input::kDown | input::kGuard; break;
        // (standing or low, as chance had it when the attack going on began: Frame)
        case Dummy::GuardRandom: bits = input::kGuard | (g_state.guardsLow ? input::kDown : 0); break;
        case Dummy::GuardAfterHit: bits = g_state.hits[dummy].since < kGuardsFor ? input::kGuard : 0; break;
        case Dummy::PunchBack: {
            // Guards; from being hit or having guarded until a moment after, presses the counter's buttons
            // every other frame (the recording, as a counter, starts above)
            const State::Exchange &own = g_state.exchange[dummy];
            const Reply counter = CounterChoice();
            if (counter != Reply::Replay && ((own.busy && own.reacted) || (own.reacted && own.freeFor < 4)) && Answers(own.by)) {
                bits = ReplyInput(counter, own.busyFor + own.freeFor + 1, onTheRight);
            } else {
                bits = input::kGuard;
            }
            break;
        }
        case Dummy::Jump: bits = input::kUp; break;
        case Dummy::Record:
            // (the tape is not part of save states: frames run ahead, which will be taken back, leave it alone)
            if (!g_state.recording && !satrecomp::RunningAhead()) { // (switched on: a new tape)
                g_state.recording = true;
                g_tape.taped = 0;
            }
            bits = g_state.playersInput;
            if (g_tape.taped < kTapeFrames && !satrecomp::RunningAhead()) {
                g_tape.input[g_tape.taped++] = onTheRight ? Mirrored(bits) : bits;
            }
            break;
        case Dummy::Replay:
            if (g_tape.taped) {
                g_state.tapeAt %= g_tape.taped;
                bits = g_tape.input[g_state.tapeAt++];
                bits = onTheRight ? Mirrored(bits) : bits;
            }
            break;
        default: break;
        }
        return bits;
    }


    // Armour as the features of the category "Armour" want it: taken off or put back, with the fighter's
    // model changed to suit by the game's own routine
    void ArmourAsWanted(satrecomp::Guest &g) {
        using satrecomp::FeatureOn;
        const bool keep = FeatureOn("armour_keep"), offBoth = FeatureOn("armour_off_both"), offDummy = FeatureOn("armour_off_dummy");
        if (!keep && !offBoth && !offDummy) {
            return;
        }
        const int dummy = DummyPlayer(g.Read8(kPlayers));
        for (int player = 0; player < 2; player++) {
            const uint32_t fighter = player ? kFighter2 : kFighter1;
            const uint8_t armour = g.Read8(fighter + fighter::kArmour);
            const uint8_t off = fighter::kUpperOff | fighter::kLowerOff;
            uint8_t wanted;
            if (keep || (offDummy && player != dummy)) {
                if (!keep) {
                    continue; // (the player's own, with the opponent's off: as the game has it)
                }
                // (no damage to it either, so that it is never ready to come off)
                g.Write32(fighter + fighter::kArmourDamage, 0);
                g.Write32(fighter + fighter::kArmourDamage + 4, 0);
                wanted = armour & ~(off | fighter::kUpperReady | fighter::kLowerReady);
            } else {
                wanted = armour | off;
            }
            if (wanted == armour) {
                continue;
            }
            g.Write8(fighter + fighter::kArmour, wanted);
            const uint8_t flags = g.Read8(fighter + fighter::kFlags);
            g.Write8(fighter + fighter::kFlags, (wanted & off) == off ? flags | fighter::kNoArmourLeft : flags & ~fighter::kNoArmourLeft);
            // What follows from it, as the game works it out when armour comes off: the weight, and the
            // parts of the body that are armoured
            const bool upperOn = !(wanted & fighter::kUpperOff), lowerOn = !(wanted & fighter::kLowerOff);
            g.Write32(fighter + fighter::kWeight, g.Read32(fighter + fighter::kWeightBody) + (upperOn ? g.Read32(fighter + fighter::kWeightUpperArmour) : 0) +
                                                      (lowerOn ? g.Read32(fighter + fighter::kWeightLowerArmour) : 0));
            const uint32_t parts = g.Read32(kArmouredParts + static_cast<uint32_t>(Byte(g, fighter + fighter::kCharacter)) * 4);
            g.Write16(fighter + fighter::kArmoured, static_cast<uint16_t>(parts & ((upperOn ? 0x00FF : 0) | (lowerOn ? 0x7F00 : 0))));
            const bool putBack = (armour & off) & ~wanted;
            // (the game's routine, with the registers kept: this is in the middle of the fighters' task)
            uint32_t kept[15];
            for (unsigned r = 0; r < 15; r++) {
                kept[r] = g.R(r);
            }
            g.R(4) = static_cast<uint32_t>(player);
            g.Call(kDressFighter);
            if (putBack) {
                // (the display's figure of the fighter only ever loses armour: the display is drawn anew)
                g.Call(kHudSetUp);
            }
            for (unsigned r = 0; r < 15; r++) {
                g.R(r) = kept[r];
            }
        }
    }

    // Where the fighters are about to be drawn: the view (fv.h) is kept for the hit overlay. (Not a rewrite:
    // the game's routine runs.)
    bool ViewSeen(satrecomp::Guest &g) {
        const uint32_t matrix = g.Read32(g.GBR() + 28);
        for (uint32_t i = 0; i < 12; i++) {
            g_state.view[i] = static_cast<float>(static_cast<int32_t>(g.Read32(matrix + i * 4))) / 65536.0f;
        }
        g_state.viewSeen = true;
        return false;
    }
    const satrecomp::Lift kViewLift[] = {{"ViewSeen", 0x06017A0E, 0x5C, 0x4A70B62AACE4B521ull, ViewSeen}};

    // An input in number pad notation ("6P", "2", "PK")
    std::string Notation(uint8_t bits) {
        static const char kDigit[3][3] = {{'7', '8', '9'}, {'4', '5', '6'}, {'1', '2', '3'}};
        const int row = bits & input::kUp ? 0 : bits & input::kDown ? 2 : 1, column = bits & input::kLeft ? 0 : bits & input::kRight ? 2 : 1;
        std::string text;
        const bool buttons = bits & (input::kGuard | input::kPunch | input::kKick);
        if (!buttons || row != 1 || column != 1) {
            text += kDigit[row][column];
        }
        if (bits & input::kGuard) {
            text += 'G';
        }
        if (bits & input::kPunch) {
            text += 'P';
        }
        if (bits & input::kKick) {
            text += 'K';
        }
        return text;
    }

    // A throw, as the game would have it now for one fighter against the other (fv.h, `reach`): the game's
    // own verdict (the bits it worked out), the reach that goes with how the other stands, and what is in
    // the way where there is no throw
    struct Throw {
        bool can;
        float apart, range;
        const char *verdict;
    };
    Throw ThrowNow(int player) {
        using satrecomp::Ram16;
        using satrecomp::Ram32;
        const uint32_t own = player ? kFighter2 : kFighter1, other = player ? kFighter1 : kFighter2;
        const uint32_t othersState = Ram32(other + fighter::kState);
        const auto within = [](uint16_t angle) { return static_cast<int16_t>(angle) >= -reach::kAngle && static_cast<int16_t>(angle) <= reach::kAngle; };
        const uint16_t towards = Ram16(own + fighter::kTowardsOther);
        // (the other's back is turned when it faces the way the thrower looks at it)
        const bool facing = within(Ram16(own + fighter::kFacing) - towards), back = within(Ram16(other + fighter::kFacing) - towards),
                   front = within(Ram16(other + fighter::kFacing) - towards + 0x8000);
        const uint32_t range = back ? reach::kBehind : (Ram32(other + fighter::kFlags) & 0x10) ? reach::kCrouching : reach::kFront;
        const uint32_t apart = Ram32(own + fighter::kDistance);
        // (the verdict: OK; or NO, the other cannot be thrown as it is; WAIT, a hit or a guard was a moment
        // ago; FAR; TURNED, one of the two does not face the way a throw wants)
        Throw result{(Ram32(own + fighter::kReach) & reach::kThrows) != 0, static_cast<float>(apart) / 65536.0f, static_cast<float>(range) / 65536.0f, "OK"};
        if (!result.can) {
            const uint8_t *noThrowsFor = satrecomp::Ram(kNoThrowsFor);
            const bool state = (othersState & reach::kCannotBeThrown) || !(othersState & reach::kCanBeThrown) || (Ram32(own + fighter::kState) & fighter::kInTheAir);
            result.verdict = state ? "NO" : noThrowsFor && *noThrowsFor ? "WAIT" : apart > range ? "FAR" : !facing || !(back || front) ? "TURNED" : "NO";
        }
        return result;
    }
    // The reach of a fighter's throw as dots on the ground: an arc at the reach, 30 degrees to each side of
    // the way the fighter faces, and the two lines back from its ends. Each dot is x, z (the hit task's
    // metres).
    std::vector<std::pair<float, float>> ThrowWedge(int player, float range, bool edges) {
        const uint32_t own = player ? kFighter2 : kFighter1;
        const float x = static_cast<float>(static_cast<int32_t>(satrecomp::Ram32(own + fighter::kPosition))) / 65536.0f,
                    z = static_cast<float>(static_cast<int32_t>(satrecomp::Ram32(own + fighter::kPosition + 8))) / 65536.0f;
        // (fv.h: a fighter with the angle 0xC000 faces along +x)
        const float faces = (static_cast<float>(satrecomp::Ram16(own + fighter::kFacing)) + 16384.0f) / 65536.0f * 6.2831853f;
        const float half = static_cast<float>(reach::kAngle) / 65536.0f * 6.2831853f;
        std::vector<std::pair<float, float>> dots;
        for (int step = -6; step <= 6; step++) {
            const float angle = faces + half * static_cast<float>(step) / 6.0f;
            dots.push_back({x + std::cos(angle) * range, z + std::sin(angle) * range});
        }
        for (float along = 0.3f; edges && along < range - 0.05f; along += 0.2f) {
            for (int side = -1; side <= 1; side += 2) {
                dots.push_back({x + std::cos(faces + half * static_cast<float>(side)) * along, z + std::sin(faces + half * static_cast<float>(side)) * along});
            }
        }
        return dots;
    }

    // The hit view: the balls of both fighters' bodies, as seen from the side of the line between them
    satrecomp::HudDiagram HitView() {
        using satrecomp::Ram32;
        satrecomp::HudDiagram diagram;
        struct Ball {
            float along, up, radius;
        } balls[2][hit::kParts];
        float hips[2][2];
        for (int player = 0; player < 2; player++) {
            for (int axis = 0; axis < 2; axis++) {
                hips[player][axis] = static_cast<float>(static_cast<int32_t>(Ram32(hit::kCentres[player] + 11 * 12 + (axis ? 8 : 0)))) / 65536.0f;
            }
        }
        // (along: from the middle between the fighters towards player 2, or away, so that left is left on the
        // screen)
        float ux = hips[1][0] - hips[0][0], uz = hips[1][1] - hips[0][1];
        const float apart = std::sqrt(ux * ux + uz * uz);
        if (apart < 0.001f) {
            ux = 1;
            uz = 0;
        } else {
            ux /= apart;
            uz /= apart;
        }
        if (Ram32(kFighter1 + fighter::kFlags) & fighter::kOnTheRight) {
            ux = -ux;
            uz = -uz;
        }
        const float mx = (hips[0][0] + hips[1][0]) / 2, mz = (hips[0][1] + hips[1][1]) / 2;
        float reach = 1.15f, top = 1.75f;
        for (int player = 0; player < 2; player++) {
            for (uint32_t part = 0; part < hit::kParts; part++) {
                const uint32_t at = hit::kCentres[player] + part * 12;
                const float x = static_cast<float>(static_cast<int32_t>(Ram32(at))) / 65536.0f, y = static_cast<float>(static_cast<int32_t>(Ram32(at + 4))) / 65536.0f,
                            z = static_cast<float>(static_cast<int32_t>(Ram32(at + 8))) / 65536.0f;
                Ball &ball = balls[player][part];
                ball = {(x - mx) * ux + (z - mz) * uz, y, static_cast<float>(static_cast<int32_t>(Ram32(hit::kRadii[player] + part * 4))) / 65536.0f};
                if (ball.radius > 0) {
                    reach = std::max(reach, std::abs(ball.along) + ball.radius + 0.1f);
                    top = std::max(top, ball.up + ball.radius + 0.1f);
                }
            }
        }
        // (as much as it takes to hold them both, at the same scale both ways; the ground a little above the
        // bottom)
        constexpr float kShape = 2.0f; // the panel's width for its height
        const float unitsHigh = std::max(top + 0.15f, reach * 2 / kShape), unitsWide = unitsHigh * kShape;
        diagram.x = 0.5f;
        diagram.y = 0.68f;
        diagram.width = 0.5f;
        diagram.height = diagram.width / kShape;
        diagram.unitsWide = unitsWide;
        diagram.unitsHigh = unitsHigh;
        for (int pass = 0; pass < 3; pass++) { // (outlines, then what attacks, then what is touched, on top)
            for (int player = 0; player < 2; player++) {
                const uint32_t fighter = player ? kFighter2 : kFighter1;
                const uint32_t attacking = Ram32(hit::kAttacking[player]), cannotBeHit = Ram32(fighter + hit::kCannotBeHit);
                uint32_t touched = 0; // (by the other's attacking parts)
                const uint32_t othersAttack = Ram32(hit::kAttacking[1 - player]);
                for (uint32_t part = 0; part < hit::kParts; part++) {
                    if (othersAttack & (1u << part)) {
                        touched |= Ram32(hit::kTouching[1 - player] + part * 4);
                    }
                }
                for (uint32_t part = 0; part < hit::kParts; part++) {
                    const Ball &ball = balls[player][part];
                    if (ball.radius <= 0) {
                        continue;
                    }
                    const uint32_t bit = 1u << part;
                    satrecomp::HudCircle circle{ball.along + unitsWide / 2, ball.up + 0.1f, ball.radius, 0, 0, 0, false};
                    if (pass == 0) {
                        if (cannotBeHit & bit) {
                            circle.r = circle.g = circle.b = 110;
                        } else if (player == 0) {
                            circle.r = 90, circle.g = 220, circle.b = 120;
                        } else {
                            circle.r = 90, circle.g = 170, circle.b = 255;
                        }
                    } else if (pass == 1 && (attacking & bit)) {
                        circle.r = 255, circle.g = 70, circle.b = 60;
                        circle.filled = true;
                    } else if (pass == 2 && (touched & bit & ~cannotBeHit)) {
                        circle.r = 255, circle.g = 220, circle.b = 60;
                        circle.filled = true;
                    } else {
                        continue;
                    }
                    diagram.circles.push_back(circle);
                }
            }
        }
        // The reach of each fighter's throw, along the ground the way it faces
        for (int player = 0; satrecomp::FeatureOn("throw_range") && player < 2; player++) {
            const Throw now = ThrowNow(player);
            for (const auto &[x, z] : ThrowWedge(player, now.range, false)) {
                satrecomp::HudCircle dot{(x - mx) * ux + (z - mz) * uz + unitsWide / 2, 0.1f, 0.025f, 150, 90, 170, true};
                if (now.can) {
                    dot.r = 255, dot.g = 90, dot.b = 255;
                }
                diagram.circles.push_back(dot);
            }
        }
        return diagram;
    }

    // A ball of the hit task's (its centre and radius in metres) where the game's view has it in the
    // picture; false if it is behind the eye
    bool OnPicture(float x, float y, float z, float radius, satrecomp::HudCircle &circle) {
        const float *m = g_state.view;
        const float wide = (4.0f / 3.0f) / satrecomp::PictureAspect(); // (a wider picture shows the same height)
        const float units = view::kUnitsAMetre;
        const float world[3] = {x * units, y * units, -z * units}; // (the hit task keeps z the other way round from the view's)
        const float vx = m[0] * world[0] + m[1] * world[1] + m[2] * world[2] + m[3], vy = m[4] * world[0] + m[5] * world[1] + m[6] * world[2] + m[7],
                    vz = m[8] * world[0] + m[9] * world[1] + m[10] * world[2] + m[11];
        if (vz < 16) {
            return false;
        }
        circle.x = 0.5f + view::kFocalX * vx / vz * wide;
        circle.y = 0.5f + view::kFocalY * vy / vz;
        circle.radius = view::kFocalY * radius * units / vz;
        return true;
    }

    // The throws' reach laid over the ground at the fighters' feet, by the game's view
    void ThrowOverlay(std::vector<satrecomp::HudCircle> &circles) {
        if (!g_state.viewSeen) {
            return;
        }
        for (int player = 0; player < 2; player++) {
            const Throw now = ThrowNow(player);
            for (const auto &[x, z] : ThrowWedge(player, now.range, true)) {
                satrecomp::HudCircle dot{0, 0, 0, 150, 90, 170, true};
                if (now.can) {
                    dot.r = 255, dot.g = 90, dot.b = 255;
                }
                if (OnPicture(x, 0, z, 0.04f, dot)) {
                    circles.push_back(dot);
                }
            }
        }
    }

    // The hit overlay: the same balls laid over the fighters, by the game's view
    std::vector<satrecomp::HudCircle> HitOverlay() {
        using satrecomp::Ram32;
        std::vector<satrecomp::HudCircle> circles;
        if (!g_state.viewSeen) {
            return circles;
        }
        for (int pass = 0; pass < 3; pass++) {
            for (int player = 0; player < 2; player++) {
                const uint32_t fighter = player ? kFighter2 : kFighter1;
                const uint32_t attacking = Ram32(hit::kAttacking[player]), cannotBeHit = Ram32(fighter + hit::kCannotBeHit);
                uint32_t touched = 0;
                const uint32_t othersAttack = Ram32(hit::kAttacking[1 - player]);
                for (uint32_t part = 0; part < hit::kParts; part++) {
                    if (othersAttack & (1u << part)) {
                        touched |= Ram32(hit::kTouching[1 - player] + part * 4);
                    }
                }
                for (uint32_t part = 0; part < hit::kParts; part++) {
                    const float radius = static_cast<float>(static_cast<int32_t>(Ram32(hit::kRadii[player] + part * 4))) / 65536.0f;
                    if (radius <= 0) {
                        continue;
                    }
                    float centre[3];
                    for (uint32_t axis = 0; axis < 3; axis++) {
                        centre[axis] = static_cast<float>(static_cast<int32_t>(Ram32(hit::kCentres[player] + part * 12 + axis * 4))) / 65536.0f;
                    }
                    satrecomp::HudCircle circle{0, 0, 0, 0, 0, 0, false};
                    if (!OnPicture(centre[0], centre[1], centre[2], radius, circle)) {
                        continue; // (behind the eye, or at it)
                    }
                    const uint32_t bit = 1u << part;
                    if (pass == 0) {
                        if (cannotBeHit & bit) {
                            circle.r = circle.g = circle.b = 130;
                        } else if (player == 0) {
                            circle.r = 90, circle.g = 255, circle.b = 130;
                        } else {
                            circle.r = 90, circle.g = 190, circle.b = 255;
                        }
                    } else if (pass == 1 && (attacking & bit)) {
                        circle.r = 255, circle.g = 60, circle.b = 50;
                        circle.filled = true;
                    } else if (pass == 2 && (touched & bit & ~cannotBeHit)) {
                        circle.r = 255, circle.g = 225, circle.b = 60;
                        circle.filled = true;
                    } else {
                        continue;
                    }
                    circles.push_back(circle);
                }
            }
        }
        return circles;
    }

    void Show() {
        std::vector<satrecomp::HudCircle> circles;
        if (satrecomp::FeatureOn("hit_overlay") && g_state.fightSeen) {
            circles = HitOverlay();
        }
        if (satrecomp::FeatureOn("throw_range") && g_state.fightSeen) {
            ThrowOverlay(circles);
        }
        satrecomp::SetHudCircles(std::move(circles));
        satrecomp::SetHudDiagram(satrecomp::FeatureOn("hit_view") && g_state.fightSeen ? HitView() : satrecomp::HudDiagram());
        std::vector<satrecomp::HudLine> lines;
        // (the longer lines start beside the list of inputs, player 1's from the left and player 2's from the
        // right, so that the two lists and the lines stay clear of each other)
        const auto beside = [&](int player, int row, const char *text) {
            lines.push_back({player == 0 ? 0.155f : 0.845f, 0.315f + 0.06f * static_cast<float>(row), player == 0 ? -1 : 1, text});
        };
        if (satrecomp::FeatureOn("frame_data") && g_state.fightSeen && g_state.result.shown < kShownFor) {
            const State::Result &result = g_state.result;
            char text[64];
            if (result.thrown && result.hitOnFrame) {
                snprintf(text, sizeof(text), "GRAB %u  DMG ON %u  %+d", result.grabbedAfter, result.hitOnFrame, result.advantage);
            } else if (result.thrown) {
                snprintf(text, sizeof(text), "GRAB %u  ESCAPED  %+d", result.grabbedAfter, result.advantage);
            } else {
                snprintf(text, sizeof(text), "HITS ON %u  %s %+d", result.hitOnFrame, result.guarded ? "GUARD" : "HIT", result.advantage);
            }
            beside(result.attacker, 0, text);
        }
        if (satrecomp::FeatureOn("move_data") && g_state.fightSeen) {
            for (int player = 0; player < 2; player++) {
                const Attack &attack = g_state.attack[player];
                if (attack.since > 2 * kShownFor) {
                    continue;
                }
                // (what the table has of the move: the attack going on may not be at its end yet)
                const auto found = g_moves.find(attack.character * 0x10000u + attack.move);
                const MoveRow row = found != g_moves.end() ? found->second : MoveRow();
                const Attack &known = attack.total || !row.attack.total ? attack : row.attack;
                const int damage = std::max(row.damage, static_cast<int>(attack.damage)), armour = std::max(row.armour, static_cast<int>(attack.armour));
                char text[64], part[2][24] = {"", ""};
                if (attack.throws) {
                    snprintf(text, sizeof(text), "THROW %03X", attack.move);
                    beside(player, 1, text);
                    if (known.total) {
                        snprintf(text, sizeof(text), "FREE AFTER %u", known.total);
                        beside(player, 2, text);
                    }
                    snprintf(text, sizeof(text), "DMG %d  ARMOUR %d", damage, armour);
                    beside(player, 3, text);
                    continue;
                }
                snprintf(text, sizeof(text), "MOVE %03X %s", attack.move, LevelName(attack.level));
                beside(player, 1, text);
                if (known.lastActive) {
                    if (known.total >= known.lastActive && known.total) {
                        snprintf(part[0], sizeof(part[0]), " REC %u", known.total - known.lastActive);
                    }
                    snprintf(text, sizeof(text), "START %u ACT %u%s", known.startUp + 1, known.lastActive - known.startUp, part[0]);
                    beside(player, 2, text);
                }
                snprintf(text, sizeof(text), "DMG %d  ARMOUR %d", damage, armour);
                beside(player, 3, text);
                part[0][0] = 0;
                if (row.hitKnown) {
                    snprintf(part[0], sizeof(part[0]), "HIT %+d  ", row.hit);
                } else if (row.knocksDown) {
                    snprintf(part[0], sizeof(part[0]), "HIT DOWN  ");
                }
                if (row.guardKnown) {
                    snprintf(part[1], sizeof(part[1]), "GUARD %+d", row.guard);
                }
                if (part[0][0] || part[1][0]) {
                    snprintf(text, sizeof(text), "%s%s", part[0], part[1]);
                    beside(player, 4, text);
                }
            }
        }
        if (satrecomp::FeatureOn("throw_range") && g_state.fightSeen) {
            for (int player = 0; player < 2; player++) {
                const Throw now = ThrowNow(player);
                char text[64];
                snprintf(text, sizeof(text), "THROW %.2f/%.2f %s", now.apart, now.range, now.verdict);
                // (thrown, and the throw is one that can be struggled out of: the presses the game has
                // counted, and how many it takes)
                const uint32_t fighter = player ? kFighter2 : kFighter1, state = satrecomp::Ram32(fighter + fighter::kState);
                if ((state & fighter::kBusy) && (state & fighter::kThrown) && satrecomp::Ram16(fighter + fighter::kStruggleMove)) {
                    snprintf(text, sizeof(text), "STRUGGLE %u OF %u", satrecomp::Ram16(fighter + fighter::kPressesCounted), satrecomp::Ram16(fighter + fighter::kStrugglePresses) + 1u);
                }
                beside(player, 5, text);
            }
        }
        if (satrecomp::FeatureOn("armour_readout") && g_state.fightSeen) {
            for (int player = 0; player < 2; player++) {
                const uint32_t fighter = player ? kFighter2 : kFighter1;
                const uint8_t armour = satrecomp::Ram(fighter + fighter::kArmour) ? *satrecomp::Ram(fighter + fighter::kArmour) : 0;
                char part[2][16];
                for (uint32_t i = 0; i < 2; i++) {
                    const int32_t left = static_cast<int32_t>(satrecomp::Ram32(fighter + fighter::kArmourStrength + i * 4)) -
                                         static_cast<int32_t>(satrecomp::Ram32(fighter + fighter::kArmourDamage + i * 4));
                    if (armour & (i ? fighter::kLowerOff : fighter::kUpperOff)) {
                        snprintf(part[i], sizeof(part[i]), "OFF");
                    } else {
                        snprintf(part[i], sizeof(part[i]), "%d", left > 0 ? left : 0);
                    }
                }
                char text[64];
                snprintf(text, sizeof(text), "ARMOUR %s / %s", part[0], part[1]);
                lines.push_back({player == 0 ? 0.25f : 0.75f, 0.195f, 0, text});
            }
        }
        if (satrecomp::FeatureOn("input_display") && g_state.fightSeen) {
            for (int player = 0; player < 2; player++) {
                const State::Inputs &inputs = g_state.inputs[player];
                // (a list down the side, the newest at the top, each with the frames it was held)
                for (uint32_t i = 0; i < inputs.count; i++) {
                    const uint32_t entry = inputs.count - 1 - i;
                    char text[32];
                    snprintf(text, sizeof(text), "%3u %-4s", inputs.frames[entry], Notation(inputs.shown[entry]).c_str());
                    lines.push_back({player == 0 ? 0.01f : 0.99f, 0.3f + 0.047f * static_cast<float>(i), player == 0 ? -1 : 1, text});
                }
            }
        }
        if (satrecomp::FeatureOn("damage_readout") && g_state.fightSeen) {
            for (int player = 0; player < 2; player++) {
                const Hits &hits = g_state.hits[player];
                if (hits.since > kShownFor) {
                    continue;
                }
                char text[64];
                if (hits.count > 1) {
                    snprintf(text, sizeof(text), "%d  %u HITS %d", hits.last, hits.count, hits.together);
                } else {
                    snprintf(text, sizeof(text), "%d", hits.last);
                }
                // (under the gauge of whoever took it: player 1's is on the left)
                lines.push_back({player == 0 ? 0.25f : 0.75f, 0.255f, 0, text});
            }
        }
        if (g_state.fightSeen) {
            char text[64];
            if (DummyMode() == Dummy::Record && g_state.recording) {
                snprintf(text, sizeof(text), g_tape.taped < kTapeFrames ? "REC %u.%u" : "REC FULL", g_tape.taped / 60, g_tape.taped % 60 / 6);
                lines.push_back({0.5f, 0.86f, 0, text});
            } else if (DummyMode() == Dummy::Replay) {
                lines.push_back({0.5f, 0.86f, 0, g_tape.taped ? "REPLAY" : "NOTHING RECORDED"});
            }
        }
        satrecomp::SetHud(std::move(lines));
    }
} // namespace

void Damage(int player, int32_t damage) {
    if (damage <= 0) {
        return;
    }
    Hits &hits = g_state.hits[player];
    // A combo: hits between which the fighter hit could not act (it was never without the busy bit, from
    // the frame after the hit before)
    if (hits.couldAct) {
        hits.count = 0;
        hits.together = 0;
    }
    hits.couldAct = false;
    hits.last = damage;
    hits.together += damage;
    hits.count++;
    hits.since = 0;
    // (the other's attack going on, or just over, did it)
    Attack &attack = g_state.attack[1 - player];
    const bool thrown = g_state.exchange[player].reacted && g_state.exchange[player].thrown && g_state.exchange[player].by == attack.serial;
    if (attack.since < kLongAgo && (attack.going || thrown || attack.since < static_cast<uint32_t>(attack.frames) + 30)) {
        attack.damage = static_cast<int16_t>(attack.damage + damage);
        Tell(attack);
    }
    if (thrown) {
        // (the frame of the throw this is: the throw's first was `since` + 1 frames before this one)
        g_state.hitOnFrame = attack.since + 2;
    }
}

int Input(satrecomp::Guest &g, int player, uint8_t fromPad) {
    return DummyInput(g, player, fromPad);
}

void InputSeen(int player, uint8_t input, bool onTheRight) {
    State::Inputs &inputs = g_state.inputs[player];
    const uint8_t bits = (onTheRight ? Mirrored(input) : input) & ~input::kOnTheRight;
    const uint8_t buttons = input::kPunch | input::kKick | input::kGuard, before = inputs.count ? inputs.shown[inputs.count - 1] : 0;
    if (bits & buttons & ~before) {
        g_state.sincePress[player] = 0;
        g_state.pressed[player] = bits;
    }
    if (inputs.count && inputs.shown[inputs.count - 1] == bits) {
        if (inputs.frames[inputs.count - 1] < 999) {
            inputs.frames[inputs.count - 1]++;
        }
        return;
    }
    if (inputs.count == kInputsShown) {
        std::copy(inputs.shown + 1, inputs.shown + kInputsShown, inputs.shown);
        std::copy(inputs.frames + 1, inputs.frames + kInputsShown, inputs.frames);
        inputs.count--;
    }
    inputs.shown[inputs.count] = bits;
    inputs.frames[inputs.count++] = 1;
}

void Frame(satrecomp::Guest &g, bool paused) {
    g_state.fightSeen = 30;
    if (paused) {
        return;
    }
    const bool fighting = Fighting(g);
    // What each fighter's armour took this frame goes to the other's attack (before the armour is put right
    // for the features that keep it whole)
    const auto armourDamage = [&](int player) {
        const uint32_t fighter = player ? kFighter2 : kFighter1;
        return g.Read32(fighter + fighter::kArmourDamage) + g.Read32(fighter + fighter::kArmourDamage + 4);
    };
    for (int player = 0; player < 2; player++) {
        const uint32_t now = armourDamage(player);
        Attack &attack = g_state.attack[1 - player];
        if (now > g_state.armourSeen[player] && attack.since < kLongAgo && (attack.going || attack.since < static_cast<uint32_t>(attack.frames) + 30)) {
            attack.armour = static_cast<int16_t>(attack.armour + (now - g_state.armourSeen[player]));
            Tell(attack);
        }
    }
    if (fighting) {
        ArmourAsWanted(g);
    }
    for (int player = 0; player < 2; player++) {
        g_state.armourSeen[player] = armourDamage(player);
        if (g_state.sincePress[player] < 999) {
            g_state.sincePress[player]++;
        }
    }
    // The attacks, for the table of moves: an attack begins when a fighter's state says it attacks (or
    // throws) in a move it was not attacking in, or in the same one begun again; it is over when the
    // fighter can act, and cut short when the fighter is hit or thrown out of it.
    for (int player = 0; player < 2; player++) {
        Attack &attack = g_state.attack[player];
        const uint32_t fighter = player ? kFighter2 : kFighter1;
        const uint32_t state = g.Read32(fighter + fighter::kState);
        const uint16_t move = g.Read16(fighter + fighter::kMove), frame = g.Read16(fighter + fighter::kMoveFrame);
        const bool busy = state & fighter::kBusy, attacking = state & fighter::kAttacking, throwing = busy && (state & fighter::kThrowing);
        if (attack.since < kLongAgo) {
            attack.since++;
        }
        if ((attacking || throwing) && (!attack.going || move != attack.move || frame < attack.lastFrame)) {
            if (attack.going) {
                attack.going = false;
                Tell(attack);
            }
            LogWhenDue(player, attack, true);
            attack = Attack();
            attack.going = true;
            attack.throws = throwing && !attacking;
            attack.character = g.Read8(fighter + fighter::kCharacter);
            attack.move = move;
            attack.level = g.Read8(fighter + fighter::kAttackLevel);
            attack.power = g.Read8(fighter + fighter::kAttackPower);
            attack.hitsFrom = g.Read16(fighter + fighter::kHitsFrom);
            attack.hitsUntil = g.Read16(fighter + fighter::kHitsUntil);
            attack.freeFrom = g.Read16(fighter + fighter::kFreeFrom);
            attack.length = g.Read16(fighter + fighter::kMoveLength);
            attack.serial = ++g_state.attacks;
            attack.since = 0;
            attack.input = g_state.pressed[player];
            if (!satrecomp::RunningAhead()) {
                g_moves[attack.character * 0x10000u + attack.move].times++;
            }
            Tell(attack);
            g_state.guardsLow = Chance() & 0x100; // (the dummy that guards by chance: how it guards this one)
        }
        if (!attack.going) {
            continue;
        }
        if (!busy) {
            attack.total = attack.frames;
            attack.going = false;
            Tell(attack);
        } else if (state & (fighter::kHit | fighter::kThrown | fighter::kKnockedDown)) {
            attack.going = false;
            Tell(attack);
        } else {
            attack.frames++;
            attack.lastFrame = frame;
            if (attacking && (state & fighter::kStartingUp) && !attack.lastActive) {
                attack.startUp = attack.frames;
            } else if (attacking && !(state & fighter::kStartingUp)) {
                attack.lastActive = attack.frames;
            }
        }
    }
    // Frame data. An exchange: a fighter is hit, guards or is thrown; it is over when both can act again,
    // and whoever was not the one hit, guarding or thrown is so many frames ahead (or behind).
    for (int player = 0; player < 2; player++) {
        State::Exchange &own = g_state.exchange[player];
        const uint32_t fighter = player ? kFighter2 : kFighter1, other = player ? kFighter1 : kFighter2;
        const uint32_t state = g.Read32(fighter + fighter::kState);
        const bool busy = state & fighter::kBusy;
        if (busy && !own.busy) {
            own.reacted = false;
            own.busyFor = 0;
        }
        if (busy && (state & (fighter::kHit | fighter::kGuarding | fighter::kThrown)) && !own.reacted) {
            own.reacted = true;
            own.thrown = state & fighter::kThrown;
            own.guarded = !own.thrown && !(state & fighter::kHit);
            own.by = g_state.attack[1 - player].serial;
            if (own.thrown) {
                // (the frame of the throw the damage comes on is not known yet: Damage; the buttons were
                // read `sincePress` - 1 frames before this one)
                g_state.hitOnFrame = 0;
                g_state.grabbedAfter = g_state.sincePress[1 - player] > 0 ? g_state.sincePress[1 - player] - 1u : 0;
            } else {
                // (this is the frame after the one it landed on)
                g_state.hitOnFrame = g.Read16(other + fighter::kMoveFrame) > 1 ? g.Read16(other + fighter::kMoveFrame) - 1u : 0;
            }
        }
        if (busy) {
            own.busyFor++;
            own.freeFor = 0;
        } else if (own.freeFor < kLongAgo) {
            own.freeFor++;
        }
        own.busy = busy;
    }
    for (int defender = 0; defender < 2; defender++) {
        State::Exchange &own = g_state.exchange[defender], &attacker = g_state.exchange[1 - defender];
        // (both can act, the defender's reaction has not been told of yet, and the attacker was not hit
        // too; an advantage of a second or more is a knockdown or the like, not told of, but for a throw)
        if (own.reacted && !own.busy && !attacker.busy && own.freeFor < kLongAgo) {
            const int32_t advantage = static_cast<int32_t>(attacker.freeFor) - static_cast<int32_t>(own.freeFor);
            const bool told = !attacker.reacted && (own.thrown || (advantage > -60 && advantage < 60));
            if (told) {
                g_state.result = {advantage, g_state.hitOnFrame, 0, own.guarded, 1 - defender, own.thrown, g_state.grabbedAfter};
            }
            // (and into the table, with the attack it was the answer to)
            Attack &attack = g_state.attack[1 - defender];
            if (!attacker.reacted && attack.serial == own.by && attack.since < kLongAgo) {
                attack.result = own.thrown ? Attack::kThrew : !told ? Attack::kKnockedDown : own.guarded ? Attack::kGuarded : Attack::kHit;
                attack.advantage = static_cast<int16_t>(std::clamp(advantage, -999, 999));
                Tell(attack);
            }
            own.reacted = false;
        }
    }
    for (int player = 0; player < 2; player++) {
        LogWhenDue(player, g_state.attack[player], false);
    }
    if (g_state.result.shown < kLongAgo) {
        g_state.result.shown++;
    }
    for (int player = 0; player < 2; player++) {
        Hits &hits = g_state.hits[player];
        if (hits.since < kLongAgo) {
            hits.since++;
        }
        // (the busy bit comes the frame after the hit: the frame of the hit itself does not count)
        if (hits.since > 1 && !(g.Read32((player ? kFighter2 : kFighter1) + fighter::kState) & fighter::kBusy)) {
            hits.couldAct = true;
        }
        if (hits.since == kRefillAfter && fighting && satrecomp::FeatureOn("gauge_refill")) {
            const uint32_t fighter = player ? kFighter2 : kFighter1;
            if (!(g.Read8(fighter + fighter::kStatus) & fighter::kKnockedOut)) {
                g.Write16(fighter + fighter::kGauge, g.Read16(kGaugeSetting + static_cast<uint32_t>(player) * 2));
            }
        }
    }
}

} // namespace fv::training

namespace satrecomp {

// After every frame the game ran
void TrainingAfterFrame() {
    using namespace fv::training;
    if (g_state.fightSeen && --g_state.fightSeen == 0) {
        // (the fight is over or left: the readout's numbers go; the dummy's tape stays)
        for (Hits &hits : g_state.hits) {
            hits = Hits();
        }
        for (State::Inputs &inputs : g_state.inputs) {
            inputs = State::Inputs();
        }
        for (Attack &attack : g_state.attack) {
            attack = Attack();
        }
    }
    // The table of moves goes to its files half a second after it last changed
    if (g_movesChanged && !RunningAhead() && ++g_movesChangedFor >= 30 && FeatureOn("move_data")) {
        WriteMoves();
    }
    Show();
}

void TrainingState(Archive &ar) {
    ar.Pod(fv::training::g_state);
}

void RegisterTraining() {
    Game &game = TheGame();
    game.features.push_back({"damage_readout", "Training", "Damage readout",
                             "Shows by each gauge what the last hit took off it, and for hits in a row their number and what they took together."});
    game.features.push_back({"input_display", "Training", "Input display",
                             "Shows each fighter's last inputs down the side, with the frames each was held: the direction as a number pad's digit (6 forward, 2 down, 5 none...), then G, P, K."});
    game.features.push_back({"gauge_refill", "Training", "Gauges refill", "A gauge is whole again a moment after its fighter was last hit."});
    game.features.push_back({"frame_data", "Training", "Frame data",
                             "After a move hits or is guarded, shows the frame it hit on and how many frames sooner (+) or later (-) than the other its fighter can act again."});
    game.features.push_back({"move_data", "Training", "Move data",
                             "Shows the last attack's level, the frame it first hits on, the frames it can hit for and those to recover, its damage, and how far ahead it "
                             "leaves you on hit and on guard. Every move done is written to moves-<character>.csv beside the settings."});
    game.features.push_back({"move_log", "Training", "Move log",
                             "Writes every move done to frame_data.csv beside the settings, a line each: what was pressed for it, its "
                             "frames, what came of it, how far ahead it left you, its damage."});
    game.features.push_back({"throw_range", "Training", "Throw range",
                             "Marks each fighter's throw reach on the ground, lit while the game would let the throw happen, with the distance and what is in the way."});
    game.features.push_back({"dummy_struggle", "Training", "Dummy struggles in throws",
                             "Thrown, the opponent presses the buttons all the while: enough presses and it breaks out of the throw unhurt. "
                             "At random: in a throw in two, as chance has it.",
                             0, {"Off", "Always", "At random"}});
    game.features.push_back({"dummy_answers_sometimes", "Training", "Dummy answers now and then",
                             "The opponent counters (Dummy counter) and does what it does as it gets up (Dummy wake-up) only one time in "
                             "two, by chance, so you cannot learn when it will."});
    game.features.push_back({"hit_overlay", "Training", "Hit overlay",
                             "Lays what the game tests for hits over the fighters: each body as 22 balls. Outlined: can be hit (grey: not now). Red: attacking. Yellow: touched by an attack."});
    game.features.push_back({"hit_view", "Training", "Hit view",
                             "A side view of what the game tests for hits: each body as 22 balls. Outlined: can be hit (grey: not now). Red: attacking. Yellow: touched by an attack."});
    game.features.push_back({"armour_readout", "Training", "Armour readout", "Shows by each gauge what is left of the upper and the lower armour, or OFF."});
    game.features.push_back({"armour_keep", "Armour", "Armour stays on", "Nobody's armour takes damage or comes off; armour that is off is put back.", kFeatureExclusive});
    game.features.push_back({"armour_off_dummy", "Armour", "Opponent without armour", "The opponent's armour is off.", kFeatureExclusive});
    game.features.push_back({"armour_off_both", "Armour", "Nobody has armour", "Both fighters' armour is off.", kFeatureExclusive});
    static const struct {
        const char *name, *title, *description;
    } kDummies[] = {
        {"dummy_stand", "Dummy: stands", "The opponent stands still."},
        {"dummy_crouch", "Dummy: crouches", "The opponent crouches."},
        {"dummy_guard", "Dummy: guards", "The opponent stands and guards."},
        {"dummy_crouch_guard", "Dummy: guards low", "The opponent crouches and guards."},
        {"dummy_guard_after_hit", "Dummy: guards after a hit", "The opponent stands, and guards for a second after each hit it takes: "
                                                               "what still hits then is a true combo."},
        {"dummy_guard_random", "Dummy: guards high or low", "The opponent guards each attack standing or crouching, by chance."},
        {"dummy_punch_back", "Dummy: guards and counters", "The opponent guards, and counters as soon as it can after guarding or being hit "
                                                          "(with a punch, or what is chosen under Dummy counter): what it beats was not safe."},
        {"dummy_jump", "Dummy: jumps", "The opponent keeps jumping."},
        {"dummy_record", "Dummy: record", "Your pad plays the opponent and what you do is recorded (up to ten seconds), while your own fighter stands. "
                                          "Switching it on starts a new recording."},
        {"dummy_replay", "Dummy: replay", "The opponent does what was recorded, over and over."},
    };
    for (const auto &dummy : kDummies) {
        game.features.push_back({dummy.name, "Dummy", dummy.title, dummy.description, kFeatureExclusive});
    }
    static const struct {
        const char *name, *title, *description;
    } kCounters[] = {
        {"counter_kick", "Counters with a kick", "The dummy that guards and counters kicks."},
        {"counter_low_kick", "Counters with a low kick", "The dummy that guards and counters crouches and kicks."},
        {"counter_throw", "Counters with a throw", "The dummy that guards and counters presses guard and punch: a throw, where it reaches."},
        {"counter_replay", "Counters with the recording", "The dummy that guards and counters does what was recorded, from its start, the moment it can act."},
    };
    for (const auto &counter : kCounters) {
        game.features.push_back({counter.name, "Dummy counter", counter.title, counter.description, kFeatureExclusive});
    }
    static const struct {
        const char *name, *title, *description;
    } kRecoveries[] = {
        {"recover_in_the_air", "Lands on its feet", "Knocked down, the opponent presses the buttons while still in the air, and lands standing where the game allows it."},
        {"recover_fast", "Gets up fast", "Knocked down, the opponent presses the buttons to be up as soon as can be."},
        {"recover_back", "Rolls back", "Knocked down, the opponent gets up rolling away from you."},
        {"recover_forward", "Rolls forward", "Knocked down, the opponent gets up rolling towards you."},
        {"recover_side", "Rolls aside", "Knocked down, the opponent gets up rolling to the side."},
        {"recover_kick", "Gets up kicking", "Knocked down, the opponent kicks as it gets up."},
        {"recover_low_kick", "Gets up kicking low", "Knocked down, the opponent rolls and kicks low as it gets up."},
        {"recover_random", "At random", "Knocked down, the opponent does one of the above, or nothing, chosen by chance each time."},
    };
    for (const auto &recovery : kRecoveries) {
        game.features.push_back({recovery.name, "Dummy recovery", recovery.title, recovery.description, kFeatureExclusive});
    }
    static const struct {
        const char *name, *title, *description;
    } kWakeUps[] = {
        {"wakeup_guard", "Guards as it gets up", "The moment it can act after being down, the opponent guards."},
        {"wakeup_punch", "Punches as it gets up", "The moment it can act after being down, the opponent punches."},
        {"wakeup_kick", "Kicks as it gets up", "The moment it can act after being down, the opponent kicks."},
        {"wakeup_low_kick", "Kicks low as it gets up", "The moment it can act after being down, the opponent crouches and kicks."},
        {"wakeup_throw", "Throws as it gets up", "The moment it can act after being down, the opponent presses guard and punch."},
        {"wakeup_replay", "Does the recording as it gets up", "The moment it can act after being down, the opponent does what was recorded, from its start."},
        {"wakeup_random", "At random as it gets up", "The moment it can act after being down, the opponent does one of the above (the recording, if there "
                                                     "is one), or nothing, chosen by chance each time."},
    };
    for (const auto &wakeUp : kWakeUps) {
        game.features.push_back({wakeUp.name, "Dummy wake-up", wakeUp.title, wakeUp.description, kFeatureExclusive});
    }
    RegisterLifts(fv::training::kViewLift, 1);
    // The training aids work in the game's own training mode only (TRAINING MODE in its main menu); elsewhere
    // they do nothing, and keep what the player chose for when it next is. (The Armour choices change the game,
    // as the cheats do, and work anywhere.)
    game.inTraining = [] {
        const uint8_t *mode = Ram(fv::kGameMode);
        return mode && *mode == fv::kModeTraining;
    };
    for (Feature &feature : game.features) {
        const std::string category = feature.category;
        if (category == "Training" || category.rfind("Dummy", 0) == 0) {
            feature.flags |= kFeatureTraining;
        }
    }
}

} // namespace satrecomp
