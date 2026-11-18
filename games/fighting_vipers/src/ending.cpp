// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
// The end of arcade mode: the staff roll (main states 13 and 14), the winner's picture after it (state 12, its
// handler at 0x0601E02A), then GAME OVER and the name entry (state 15, 0x060397F6) or the title (state 5).
//
// The staff roll's song is the sound driver's own music, not an audio track. At the picture the game sends the
// command that fades the music out (0xA0023CFF, the one it sends at the end of every round), but that command
// fades only the CD audio (it sets two of the sound chip's DSP coefficients, 0x70C and 0x710, to 0x1C90), so the
// song goes on at full volume. It stops only when the driver changes sound banks (0xA00001FF), which the game asks
// for at GAME OVER together with track 6, the GAME OVER music: the song is still heard over its first quarter
// second. The original does this too (the reference build, Ymir's sound CPU and chip, sounds the same).
//
// `ending_fade` (Enhanced): over the picture the song fades out by the chip's master volume (MVOL, the low four bits
// of register 0x400, 3 dB a step; the driver sets it to 15 at start-up and leaves it), the bank is changed once it
// is silent, and the volume is put back before GAME OVER. All of it follows the picture's countdown (the frame
// loop's 0x060E0060, 328 down to below 0) and nothing else, so a save state taken in the middle goes on from where it
// was, and run-ahead's frames taken back leave nothing behind.
#include "game.h"
#include "lift.h"

namespace satrecomp {
namespace {

constexpr uint32_t kCountdown = 0x060E0060;  // long: frames left of the screen (the frame loop takes one off a frame)
constexpr uint32_t kStage = 0x060E0004;      // byte: 9 once the last fight is won (state 12 then goes to state 15)
constexpr uint32_t kLastStage = 9;
constexpr uint32_t kSoundCommand = 0x25A03C20; // the driver's mailbox (0x060131A8 waits for it to be empty)
constexpr uint32_t kChangeBank = 0xA00001FF;   // what the game sends at GAME OVER
constexpr uint32_t kMasterVolume = 0x25B00401; // (the register's low byte; its upper bits read only)
constexpr uint8_t kFullVolume = 15;

// The countdown at which the fade starts (the picture's 10th frame, where the game checks for 0x013E too), the
// frames a step, when the bank is changed, and when the volume goes back up
constexpr int32_t kFadeFrom = 0x013E, kFramesAStep = 10, kChangeAt = 160, kRestoreAt = 2;

// The winner's picture, every frame (state 12): first the fade, then the game's own handler
bool EndingPicture(Guest &g) {
    if (!FeatureOn("ending_fade") || g.Read8(kStage) < kLastStage) {
        return false; // (a lost game's picture: the music is CD audio, which the game's command fades)
    }
    const int32_t left = static_cast<int32_t>(g.Read32(kCountdown));
    if (left > kFadeFrom) {
        return false;
    }
    uint8_t volume = kFullVolume;
    if (left > kRestoreAt) {
        const int32_t steps = (kFadeFrom - left) / kFramesAStep;
        volume = static_cast<uint8_t>(steps >= kFullVolume ? 0 : kFullVolume - steps);
    }
    g.Write8(kMasterVolume, volume);
    // (on that one frame, as 0x060131A8 sends it; the game sends nothing else over the picture, so the mailbox is
    // empty. Nothing but the countdown decides, for save states and run-ahead.)
    if (left == kChangeAt && g.Read32(kSoundCommand) == 0) {
        g.Write32(kSoundCommand, kChangeBank);
    }
    return false;
}

const Lift kLifts[] = {
    {"EndingPicture", 0x0601E02A, 0xA4, 0x900A995B67E41F03ull, EndingPicture},
};

} // namespace

void RegisterEnding() {
    TheGame().features.push_back({"ending_fade", "Game", "Staff roll's song fades out",
                                  "After the staff roll the song fades out over the winner's picture, rather than going on into the GAME OVER music.",
                                  kFeatureEnhanced});
    RegisterLifts(kLifts, sizeof(kLifts) / sizeof(kLifts[0]));
}

} // namespace satrecomp
