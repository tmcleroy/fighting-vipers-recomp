// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
// Fighting Vipers: what is known of the game's memory, by name. Addresses are the Saturn's; values there are
// big-endian. (Found by watching the game run and by reading its code; PROGRESS.md says how.)
#pragma once

#include "lift.h"

#include <cstdint>

namespace fv {

// A fighter's record. The game's code keeps the address of the one it is working on in r12.
constexpr uint32_t kFighter1 = 0x060627D4, kFighter2 = 0x060646D4;
namespace fighter {
    constexpr uint32_t kStatus = 0x0002;       // byte; bit 3: knocked out
    constexpr uint32_t kGauge = 0x0054;        // word: what is left of the gauge, 250 when full
    constexpr uint32_t kFlags = 0x0000;        // long; bit 27: on the right, facing left
    constexpr uint32_t kOnTheRight = 0x08000000;
    // Armour, on the upper and the lower body. Each has a strength (the fighter's own: 300 and 100, 500 and
    // 200...) and the damage it has taken, which hits add to (0x06025AC4) and which is kept from round to
    // round. Once the damage is the strength the armour is marked as ready to come off (0x06025E20), and the
    // next hit of the right kind takes it off (0x06025BF6 on).
    constexpr uint32_t kArmourStrength = 0x0040; // two longs: upper, lower
    constexpr uint32_t kArmourDamage = 0x0048;   // two longs
    constexpr uint32_t kArmour = 0x005F;         // byte (the last of a long): the bits below
    constexpr uint8_t kUpperReady = 0x01, kLowerReady = 0x02, kUpperOff = 0x04, kLowerOff = 0x08;
    constexpr uint8_t kNoArmourLeft = 0x10;      // (in the first byte of kFlags: both are off)
    constexpr uint32_t kNumber = 0x0004;         // byte: 0 or 1, the player
    constexpr uint32_t kCharacter = 0x0059;      // byte: which character
    // What follows from the armour that is on (the game works these out when armour comes off, 0x06012EA0 and
    // 0x06025F14): the fighter's weight, the body's with the weight of each armour on; and the parts of the
    // body that are armoured, the character's (a table of longs at kArmouredParts) less those of armour off
    constexpr uint32_t kWeight = 0x04C0;         // long
    constexpr uint32_t kWeightUpperArmour = 0x0608, kWeightLowerArmour = 0x060C, kWeightBody = 0x0610; // longs
    constexpr uint32_t kArmoured = 0x061C;       // word: bits 0-7 the upper body's parts, 8-14 the lower's
    // What the fighter is doing: the move and the frame of it, and the bits of the state (seen: a punch is
    // kBusy | kStartingUp | kAttacking for the frames before it hits, loses kStartingUp on the frame it
    // hits, and is kBusy | kRecovering after; whoever it hits is kBusy | kHit, whoever guards it kBusy |
    // kGuarding. A fighter acts on the frame after kBusy goes: tried with a dummy pressing punch out of
    // being hit.)
    constexpr uint32_t kMove = 0x0050;         // word
    constexpr uint32_t kMoveFrame = 0x0052;    // word: from 1
    constexpr uint32_t kState = 0x002C;        // long
    constexpr uint32_t kBusy = 0x01, kStartingUp = 0x02, kRecovering = 0x04, kAttacking = 0x100, kStanding = 0x200, kGuarding = 0x2000,
                       kHit = 0x20000;
    // Knocked down: kKnockedDown from the hit that does it, kOnTheGround from landing until the fighter is
    // up again. Lying there (move 0x6A) takes 86 frames with nothing pressed; a direction held, 46, and the
    // fighter rolls that way as it gets up (back 0x19C, forward 0x1A4, up 0x19E, down 0x1A0); a button
    // pressed again and again, less: kick or guard 62 (and the kick comes as the fighter is up), punch 22
    // with a quick way up of its own (0x19A); down with kick, a low kick out of the roll (0x201). The three
    // buttons pressed again and again from the hit, while still in the air, have the fighter land on its
    // feet (0x281) and not lie down at all.
    constexpr uint32_t kKnockedDown = 0x4000, kOnTheGround = 0x10000;
    // More of the state's bits: crouching, in the air (tried with the dummy that crouches and the one that
    // jumps); throwing and being thrown (guard and punch pressed beside a standing dummy: the thrower's
    // state is 0x40063, the other's 0x80063).
    constexpr uint32_t kCrouching = 0x08, kInTheAir = 0x10, kThrowing = 0x40000, kThrown = 0x80000;
    // Where the fighter is, and how it stands to the other (0x06010C40 works these out each frame, for each
    // fighter in turn). The place is that of the hips (the hit task's ball 11). Angles are words, 0x10000 a
    // full turn: a fighter facing along +x has 0xC000, along -x 0x4000 (the angle of (z, x) less a quarter turn).
    constexpr uint32_t kPosition = 0x00DC;     // three longs: x, y (up), z; 16.16, metres
    constexpr uint32_t kFacing = 0x049C;       // word: the way the fighter faces
    constexpr uint32_t kTowardsOther = 0x049E; // word: the way from the fighter to the other
    constexpr uint32_t kDistance = 0x04D4;     // long: how far apart the two are (x and z only); 16.16, metres
    // What the fighter could do to the other where they stand now (0x06010C40 again; the commands' scripts
    // test these bits, 0x06015F60): see `reach`, below
    constexpr uint32_t kReach = 0x0578;        // long
    // What the fighter is asked to do and is doing. A request is a long: its first byte the kind (`action`,
    // below), its last word a move. The reading of commands leaves one (0x06015CEE); 0x0600C974 gives the
    // other fighter the request that answers a throw; 0x0600CC02 takes a request up as the action, and the
    // action's own routine (the table at 0x06011608, two longs a kind) runs each frame, by steps.
    constexpr uint32_t kRequest = 0x003C;      // long
    constexpr uint32_t kAction = 0x006C;       // long
    constexpr uint32_t kActionStep = 0x005A;   // byte
    // Requests that go before the commands' (throws.cpp, PendingRequest): one the game's routines force for
    // the next frame (being knocked down after a throw, say), and one put off until a frame of the move
    constexpr uint32_t kForcedRequest = 0x0064;   // long
    constexpr uint32_t kDeferredRequest = 0x0068; // long
    constexpr uint32_t kDeferredUntil = 0x0502;   // word: the frame of the move (kMoveFrame) it waits for
    // The move being done, as its data has it (0x06011BF4 starts a move and copies these from the move's
    // records: `move`, below). Those of an attack keep the last attack's values afterwards.
    constexpr uint32_t kMoveLength = 0x063C;   // word: frames
    constexpr uint32_t kNextMove = 0x063E;     // word: the move after it (0xFFFF: none)
    constexpr uint32_t kHitsFrom = 0x0644;     // word: the first frame of the move on which it can hit
    constexpr uint32_t kHitsUntil = 0x0646;    // word: the frame after the last on which it can
    constexpr uint32_t kFreeFrom = 0x0648;     // word: the frame from which the fighter can act again
    constexpr uint32_t kAttackPart = 0x065C;   // byte: which parts of the body attack (the table at 0x060094E4)
    constexpr uint32_t kAttackLevel = 0x065D;  // byte: `level`, below
    constexpr uint32_t kAttackPower = 0x065E;  // byte: what damage is worked out from (and the armour's)
    constexpr uint32_t kAttackFlags = 0x0660;  // word
    // A move that is struggled out of (being thrown): with more than kStrugglePresses presses counted
    // (kPressesCounted: one a frame at most, while the state has kBusy and kThrowing or kThrown) by its end,
    // kStruggleMove follows in place of kNextMove (0x0601245E). That is the way out of a throw: thrown by
    // guard and punch (move 0x16, 52 frames, 18 presses) a fighter that pressed 19 times or more goes on
    // to 0x1AA, takes no damage and is on its feet; one that did not goes on to 0x1AB, takes the damage 22
    // frames later and is knocked down.
    constexpr uint32_t kStruggleMove = 0x0690, kStrugglePresses = 0x0692; // words
    constexpr uint32_t kThrownWaitsFor = 0x0696; // word: the frame of being thrown that its routine (0x0600F110) waits for first
    constexpr uint32_t kInput = 0x1224;        // byte: see input, below
    constexpr uint32_t kHistoryAt = 0x1226;    // byte: the history's entry for the next frame
    constexpr uint32_t kHistoryReaders = 0x1227; // four bytes: entries the command reader has got to
    constexpr uint32_t kInputFlags = 0x122C;   // long; bit 3: start the input afresh; bit 1: as bit 0 of kState or
                                               // bit 10 of kFlags this frame; bit 2: as bit 1 the frame before
    constexpr uint32_t kForward = 0x1230, kBackward = 0x1234; // longs: which of input::kRight and kLeft is which now
    constexpr uint32_t kCommand = 0x1240;      // long, with a count at 0x123A (word): the command being read
    constexpr uint32_t kHistory = 0x1268;      // (input, above)
    constexpr uint32_t kPressesCounted = 0x06C8; // word: presses while bit 0 and bit 18 or 19 of kState are set
    constexpr uint32_t kHitDamage = 0x11EC;    // word: the damage of the hit being taken
    constexpr uint32_t kNoDamage = 0x1DB1;     // byte: not zero while the gauge is not to go down
    constexpr uint32_t kHitFlags = 0x1DB2;     // byte; bit 0 is cleared when damage is taken
    constexpr uint32_t kLastDamage = 0x1E04;   // word: the damage taken last
    constexpr uint8_t kKnockedOut = 0x08;
    constexpr uint8_t kNoCommands = 0x04;      // (of kStatus: the commands' requests are dropped; set while being thrown)
    constexpr int kGaugeFull = 250;
} // namespace fighter

// Bits of fighter::kReach, as 0x06010C40 sets them for a fighter (the thrower) and the other. A throw wants
// the thrower on the ground, the other neither down, hit nor in the air, the thrower facing the other
// within 30 degrees (0x1554), and then:
//   the other facing the thrower (within 30 degrees) and standing, walking, guarding or recovering (bits
//   0x04003614 of its state): kThrowFront at up to 1.7 m (0x1B333) from a standing one, kThrowCrouching at
//   up to 1.6 m (0x1999A) from a crouching one (bit 4 of the last byte of its flags);
//   the other with its back to the thrower: kThrowBehind at up to 1.4 m (0x16666), kThrowBehindCrouching.
// A fighter that is attacking (starting up or hitting) has none of those state bits: it cannot be thrown.
// For kNoThrowsFor frames after a hit or a guard (14) there is no throw at all.
// kOwnWall and kOthersWall: within a metre of a wall of the stage and turned to it (the throws there differ).
namespace reach {
    constexpr uint32_t kThrowFront = 0x02, kThrowCrouching = 0x04, kThrowBehind = 0x08, kThrowBehindCrouching = 0x10;
    constexpr uint32_t kThrows = kThrowFront | kThrowCrouching | kThrowBehind | kThrowBehindCrouching;
    constexpr uint32_t kOwnWall = 0x100, kOthersWall = 0x200;
    constexpr uint32_t kFront = 0x1B333, kCrouching = 0x1999A, kBehind = 0x16666; // 16.16, metres
    constexpr uint16_t kAngle = 0x1554;
    constexpr uint32_t kCanBeThrown = 0x04003614;  // (bits of the other's state: one of them)
    constexpr uint32_t kCannotBeThrown = 0x4003C000 | fighter::kInTheAir; // (none of them)
} // namespace reach

// Kinds of request and action (the first byte of fighter::kRequest and kAction) that were met
namespace action {
    constexpr uint8_t kNone = 0x01;  // (asked for each frame nothing else is)
    // A throw and being thrown by it. 0x10 is the throw of guard and punch; 0x25, 0x27 and 0x2B are other
    // throws, answered by 0x26, 0x28 and 0x2C (not seen done).
    constexpr uint8_t kThrow = 0x10, kThrown = 0x11;
} // namespace action

// The moves' data: a table of pointers by move number (the low 12 bits), each to eight bytes and then
// records, the first byte of each its type, until a type of 0 or 8 (0x06012428 looks a type up). The
// length of a record is in the table at kRecordSizes, by type. Seen: type 2, the frames (words at 2, 4, 6:
// fighter::kHitsFrom, kHitsUntil, kFreeFrom); type 4, an attack (bytes at 1, 2: kAttackPart, kAttackPower;
// at 3: the low half kAttackLevel; word at 6: kAttackFlags); type 5, a throw (word at 2: the move of
// whoever is thrown); type 6, being thrown (word at 2: fighter::kThrownWaitsFor).
namespace move {
    constexpr uint32_t kTable = 0x0606D304, kRecordSizes = 0x0606C6EE;
} // namespace move

// fighter::kAttackLevel, by what the routine that works out a hit (0x06024DAE on) does with it:
//   0  misses a crouching fighter; guarded standing                        high
//   1  guarded standing; hits a crouching guard, for three quarters        middle
//   2  hits a standing guard; guarded crouching                            low
//   3  as 1, but a crouching guard takes all of it                         middle
//   4  the only kind that hits a fighter lying down; guarded either way    on the ground
//   5, 6  as 2                                                             low
namespace level {
    constexpr uint8_t kHigh = 0, kMiddle = 1, kLow = 2, kMiddleHeavy = 3, kGround = 4;
} // namespace level

// The round's clock. The game counts 64 frames to one of its seconds.
constexpr uint32_t kRoundTime = 0x060E005C;    // long: time left in 1/64 seconds (30 seconds: 1920); not positive: over
constexpr uint32_t kRoundTimeFull = 30 * 64;
constexpr uint32_t kTimeTaken = 0x060E006C;    // long: time gone by in 1/100 seconds
constexpr uint32_t kTimeTakenStep = 0x0609D6BC; // byte: where in kCentisecondSteps the count is (it goes down)
constexpr uint32_t kCentisecondSteps = 0x0605F720; // 16 bytes that add up to 25: 100/64 a frame
constexpr uint32_t kClockSwitches = 0x060E0016; // byte
constexpr uint8_t kClockCountsDown = 0x01, kClockCountsTimeTaken = 0x02;
constexpr uint32_t kFightFlags = 0x060E0058;    // long; bit 5: nothing of the round moves on
constexpr uint32_t kFightPaused = 0x060E005B;   // (its last byte)
constexpr uint8_t kFightPausedBit = 0x20;
// A count of frames that FightersTask takes down, 14 from each hit or guard (seen: a guarded punch). While
// it runs nobody is in reach of a throw (0x06010EE0, in the routine that works fighter::kReach out).
constexpr uint32_t kNoThrowsFor = 0x060E0022;   // byte
constexpr uint32_t kRoundEvents = 0x060E0067;   // byte; bit 0: time is up
constexpr uint8_t kTimeUp = 0x01;

constexpr uint32_t kRoundTimeSetting = 0x0608425C; // long: the time a round starts with
constexpr uint32_t kStartCountdown = 0x060E0060; // long: frames until the round starts
// Bits of the long at 0x060E0064 (kRoundEvents is its last byte)
constexpr uint32_t kRoundFlags = 0x060E0064;
constexpr uint32_t kRoundTimeUp = 0x01;      // the clock ran out
constexpr uint32_t kRoundMatchOver = 0x02;   // somebody has the rounds a match takes
constexpr uint32_t kRoundSuddenDeath = 0x10; // the deciding round after a drawn match: half the time, one point each
constexpr uint32_t kRoundJoinIn = 0x20;      // the other player may come in by pressing Start
constexpr uint32_t kFightSwitches = 0x060E0070; // long; what its bits do is not known yet
constexpr uint32_t kFightSwitches2 = 0x060E0072, kFightSwitches3 = 0x060E0073; // (its bytes)

// What is being played
constexpr uint32_t kGameMode = 0x060E0002;  // byte
// (1 was seen played; 3 and 4 are named by what the round's code does for them: 3 keeps the records of who
// beat whom, 4 has teams with the winner staying on. 5 is the main menu's TRAINING MODE, seen chosen there:
// its fight is a state of its own, state::kTraining. 7 counts no rounds and ends none; it was seen played
// from the PLAYBACK MODE screen (a recorded or an exhibition match), and was once taken for training.)
constexpr uint8_t kModeArcade = 1, kModeVersus = 3, kModeTeam = 4, kModeTraining = 5, kModePlayback = 7;
constexpr uint32_t kPlayers = 0x060E0001;   // byte: 1 player 1 against the game, 2 player 2 against it, 3 both, 0 neither
constexpr uint32_t kWinsNeeded = 0x00200E28; // byte: the rounds a match takes (a setting)
constexpr uint32_t kWins = 0x060E0008;      // two bytes: the rounds each player has
constexpr uint32_t kDown = 0x060E000F;      // byte: who is knocked out (bit 0 player 1, bit 1 player 2)
constexpr uint32_t kRoundResult = 0x060E000C; // byte: 1 player 1 won the round, 2 player 2, 3 a draw
constexpr uint32_t kRoundWinner = 0x060E0011; // byte: the same, but in a draw whoever has the match by it (3: nobody)
constexpr uint32_t kRoundEndedBy = 0x060E000D; // byte: 0 a knockout, 2 time
constexpr uint32_t kTimeUpWinner = 0x060945A8; // word: who had more of their gauge left when time ran out (1, 2; 0 neither)
constexpr uint32_t kRound = 0x060E000A;     // byte: the round being fought, from 1
constexpr uint32_t kStage = 0x060E0005;     // byte
constexpr uint32_t kModeTable = 0x0605F378; // four bytes a mode
constexpr uint32_t kLastWinner = 0x06095809; // byte: 0 nobody yet, 1 player 1

// The fight's state: which routine of the table at 0x0605F1DC runs each frame (0x0601C4C4 calls it)
constexpr uint32_t kFightState = 0x06090768; // long
constexpr uint32_t kFightStates = 0x0605F1DC; // the table: for each state its routine (long) and a byte, 8 bytes
namespace state {
    constexpr uint32_t kJoinIn = 17;
    constexpr uint32_t kWinner = 12; // (the winner is shown: up to 320 frames by kStartCountdown)
    constexpr uint32_t kSetUp = 3, kCountdown = 4, kBegin = 5, kBeginWait = 6, kLetGo = 7, kFighting = 8, kRoundOver = 9, kRoundOverWait = 10,
                       kAfterRound = 11, kAfterRoundPlayback = 13;
    // The training mode's (kModeTraining): 21 as it begins, then 22 for as long as the fighters fight (seen)
    constexpr uint32_t kTrainingBegin = 21, kTraining = 22;
} // namespace state

// Saved settings, in low work RAM
constexpr uint32_t kGaugeSetting = 0x00200E24;  // two words: the gauge each player starts a round with
constexpr uint32_t kLongGauge = 0x002000A0;     // two bytes: not zero, that player's gauge is 400
constexpr int kGaugeLong = 400;

// What a fighter is told to do this frame: a byte in its record (fighter::kInput). The fight's input task
// (src/fightinput.cpp) fills it in from the pad's buttons where a person plays, or leaves what the game's
// own play (the tasks before it, 0x0601E680 and 0x0601E784) put there; keeps a history of it; and
// FightersTask clears it at the end of the frame.
namespace input {
    constexpr uint8_t kPunch = 0x01, kKick = 0x02, kGuard = 0x04, kDown = 0x10, kUp = 0x20, kRight = 0x40, kLeft = 0x80;
    constexpr uint8_t kOnTheRight = 0x08; // (put in by the input task: the fighter is on the right, facing left)
    // The button settings, a table for each player: button n of the pad (bit n + 3 of its word) gives the
    // bits of the long at n; a negative long ends the table
    constexpr uint32_t kButtonTable[2] = {0x0607A808, 0x0607A840};
    // A long for each player; bit 0: the input does not come from the pad (seen clear)
    constexpr uint32_t kNotFromPad[2] = {0x06060154, 0x06061454};
    // The history: the last 64 frames' input, four bytes each, in a ring in the fighter's record
    constexpr uint32_t kHistoryFrames = 64;
    constexpr uint32_t kHeld = 0, kHeldOrLastDirection = 1, kPressed = 2, kReleased = 3;
} // namespace input

// A player's part in the fight: a record for each (the game's code has it in r14 where it works on a
// player's input)
constexpr uint32_t kPlayer1 = 0x06062754, kPlayer2 = 0x06062794;
namespace player {
    constexpr uint32_t kFlags = 0;      // long; bit 0 (player 1's record) or 1 (player 2's): may act (the round's
                                        // loser may not); bit 2: the input and the side are given from elsewhere
    constexpr uint32_t kNumber = 4;     // byte: 0 or 1
    constexpr uint32_t kStack = 20;     // two longs: the stack pointer during each player's input step
    constexpr uint32_t kCommands = 28;  // long: where the command reader is in the player's command table
    constexpr uint32_t kReleased = 36;  // byte: the input let go of this frame
    constexpr uint32_t kCommandTables[2] = {0x060A4000, 0x060A5000};
} // namespace player

// Hits. The game's hit task (0x06004BB8, each frame before the fighters') has each fighter's body as 22 balls
// (0, 1 the trunk; 2 the head; 3-6 and 7-10 the arms; 11 the hips; 12-15 and 16-19 the legs; 20, 21 are
// not used by every character), and finds which of one fighter's touch which of the other's.
namespace hit {
    constexpr uint32_t kParts = 22;
    constexpr uint32_t kCentres[2] = {0x060E01D8, 0x060E03E8}; // for each part three longs: x, y (up), z; 16.16, metres or so
    constexpr uint32_t kRadii[2] = {0x060092D0, 0x06009328};   // a long for each part (0x06004A44 sets them: twice the
                                                              // character's own for the parts that are attacking)
    constexpr uint32_t kAttacking[2] = {0x060672A4, 0x060672A8}; // long: the parts the fighter's move attacks with
    constexpr uint32_t kTouching[2] = {0x060E00D0, 0x060E0128};  // for each part a long: the other's parts it touches
    constexpr uint32_t kCannotBeHit = 0x0560;                    // (in the fighter's record) long: parts that cannot be hit now
    constexpr uint32_t kPartsHit = 0x0564;                       // long: the parts hit by the other's attack this frame
} // namespace hit

// The view. Where the fighters are about to be drawn (0x06017A0E, in the task at 0x06017B88) the 3D
// library's current matrix (its address is the long at 28 from GBR) takes the world to the view: three rows
// of three longs and a fourth, 16.16; x to the right, y down, z away. A metre of the hit task's is 300 of
// its units, whoever fights (the origin of a body part as drawn, the last three longs of its matrix in the
// fighter's record from 0x1CC on, is 300 times the centre of the ball there), and the hit task's z is the
// other way round. The vertex routine (0x0602331C) then multiplies x and y by the longs of the table at
// kPerspective for the depth (two longs for each 8 units of depth: about 682 / z and 560 / z, in 704 by 448
// dots).
// The fight's camera. Each frame the view is made at 0x06005DEE (r14 the camera's object): the 3D library's matrix
// made a unit one (0x06052024, returning to kViewMade), turned about x by 0x8000 less the object's word at kPitch
// (0x0605280C) and about y by its word at kYaw the other way round (0x0605289C), and moved by the eye's place
// (0x0605291C: x and y times -300, z times 300). The camera's logic (from 0x06028700; the mode at 0x060E0005)
// sets them. The library's turns multiply the matrix on the right: Rx = [1 0 0; 0 c -s; 0 s c],
// Ry = [c 0 s; 0 1 0; -s 0 c], angles in 65536ths of a turn.
namespace camera {
    constexpr uint32_t kUnitMatrix = 0x06052024; // the library's: the current matrix made a unit one
    constexpr uint32_t kViewMade = 0x06005DF6;   // where the view's making goes on after it
    constexpr uint32_t kMove = 0x0605291C;       // the library's: the current matrix moved by r4, r5, r6
    constexpr uint32_t kViewMoved = 0x06005E58;  // where the view's making goes on after its move by the eye
    constexpr uint32_t kPitch = 24, kYaw = 26;   // (in the camera's object) words
    constexpr uint32_t kEye = 0x0605FBE0;        // three longs, 16.16 metres: x, y (up), z as the hit task's
} // namespace camera

namespace view {
    constexpr float kUnitsAMetre = 300.0f;
    constexpr uint32_t kPerspective = 0x0608064C;
    constexpr float kFocalX = 682.0f / 704.0f, kFocalY = 560.0f / 448.0f; // (as parts of the 4:3 picture)
} // namespace view

// The control pads as the game keeps them (0x0602BA24 fills them in each frame, src/input.cpp): 0x18 bytes
// each, player 1's first
constexpr uint32_t kPads = 0x060943A0;
namespace pad {
    constexpr uint32_t kSize = 0x18;
    constexpr uint32_t kHeld = 2;         // word: the buttons held
    constexpr uint32_t kPressed = 4;      // word: the buttons that went down this frame
    constexpr uint8_t kStartHigh = 0x08;  // (Start, in the word's first byte)
} // namespace pad

// The task list: records of 0x1C bytes; the routine at +12 is called every frame while +8 is 1
// (0x06004560 goes through them)
namespace task {
    constexpr uint32_t kOn = 8, kRoutine = 12;
    constexpr uint32_t kClock = 0x060857E8, kFighters = 0x060858E4;
    // (records with no routine of their own: kFighters does a fighter's frame while that fighter's is on;
    // the first long of each is its fighter's record)
    constexpr uint32_t kFighter1 = 0x06085858, kFighter2 = 0x06085874;
    constexpr uint32_t kDuringFightA = 0x06085938, kDuringFightB = 0x06085954; // (what they do is not known)
} // namespace task

// Routines of the game's not rewritten yet
constexpr uint32_t kDrawRoundTime = 0x0602F094;
constexpr uint32_t kFighterFrame = 0x060106D8;        // a fighter's frame (r12: the fighter, r13: the other)
constexpr uint32_t kFighterAgainstOther = 0x06010C40; // what a fighter does to the other (the same)
constexpr uint32_t kArmouredParts = 0x060671B8;
constexpr uint32_t kHudSetUp = 0x0602438C;     // draws the fight's display anew (the round's set-up calls it)
constexpr uint32_t kDressFighter = 0x0600B124; // (r4: the player) the fighter's model with the armour it has, by fighter::kArmour
constexpr uint32_t kAnnounce = 0x060131A8;      // (r4: a sound command, 0xA011nnFF: the announcer's line nn)

// Small things the rewritten routines share
inline void StartTask(satrecomp::Guest &g, uint32_t record, uint32_t routine) {
    g.Write32(record + task::kOn, 1);
    g.Write32(record + task::kRoutine, routine);
}
inline void StopTask(satrecomp::Guest &g, uint32_t record) {
    g.Write32(record + task::kOn, 0);
}
inline void SetBits(satrecomp::Guest &g, uint32_t address, uint8_t bits) {
    g.Write8(address, g.Read8(address) | bits);
}
inline void ClearBits(satrecomp::Guest &g, uint32_t address, uint8_t bits) {
    g.Write8(address, g.Read8(address) & ~bits);
}
inline int8_t Byte(satrecomp::Guest &g, uint32_t address) {
    return static_cast<int8_t>(g.Read8(address));
}

} // namespace fv
