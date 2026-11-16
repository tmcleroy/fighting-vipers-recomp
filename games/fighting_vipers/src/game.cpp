// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
// Fighting Vipers: the routines of the game that are replaced by hand-written C++ (see PROGRESS.md).
#include "game.h"
#include "lift.h"
#include "shared_lifts.h"
#include "state.h"

namespace satrecomp {

void RegisterFightLifts(); // fight.cpp
void RegisterRoundLifts(); // round.cpp
void RegisterThrowLifts(); // throws.cpp
void RegisterFightInputLifts(); // fightinput.cpp
void RegisterWidescreen(); // widescreen.cpp
void WidescreenAfterFrame();
void RegisterCheats();     // cheats.cpp
void RegisterTraining();   // training.cpp
void RegisterCamera();     // camera.cpp
void CameraAfterFrame();
void TrainingAfterFrame();
void TrainingState(Archive &ar);
void RegisterJapanese();   // japanese.cpp
void JapaneseAfterFrame();

namespace {

// File loading, CD music and controllers: routines of Sega's libraries and of AM2's engine that Fighters
// Megamix carries too, replaced by the toolkit's shared C++ (satrecomp/runtime/shared_lifts.h)
void RegisterShared() {
    using namespace shared;
    // (LoadFile, kAm2LoadFile at 0x06029B44: japanese.cpp's, which loads as it does)
    Register(kAm2ReadPads, 0x0602BA24);
    Register(kGfsOpen, 0x06044D78);
    Register(kGfsStart, 0x06045020);
    Register(kGfsPoll, 0x06045394);
    Register(kGfsClose, 0x06045308);
    // The routine 0x0602B7AC asks after the file being streamed (the library's answer returns to 0x0602B7BC). A
    // loading routine waits on it, but the frame loop also calls it every frame (its jsr at 0x0602263E, returning to
    // 0x06022642) and throws the answer away. When the frame loop asked first, it took the one "complete" that the
    // stream was waiting for: the credits after the last match stream a file this way (MOT_BAH.BIN) and waited
    // forever. The frame loop's asking only looks. (0x0602B7AC keeps the frame loop's return address on top of
    // the stack, under r8.)
    PollOnlyLooks([](Guest &g) { return g.PR() == 0x0602B7BC && g.Read32(g.R(15)) == 0x06022642; });
    Register(kCdPlay, 0x0604490C);
    Register(kCdSeek, 0x060449D0);
    Register(kCdGetCurStat, 0x06044384);
    Register(kPerSendCommand, 0x06058934);
}

} // namespace

void RegisterGame() {
    RegisterShared();
    RegisterFightLifts();
    RegisterRoundLifts();
    RegisterThrowLifts();
    RegisterFightInputLifts();
    RegisterWidescreen();
    RegisterCheats();
    RegisterTraining();
    RegisterCamera();
    RegisterJapanese();
    TheGame().frame = [] {
        JapaneseAfterFrame();
        CameraAfterFrame();
        TrainingAfterFrame();
        WidescreenAfterFrame();
    };
    TheGame().state = [](Archive &ar) {
        shared::State(ar);
        TrainingState(ar);
    };
}

} // namespace satrecomp
