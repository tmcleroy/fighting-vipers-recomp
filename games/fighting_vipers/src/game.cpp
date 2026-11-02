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

namespace {

// File loading, CD music and controllers: routines of Sega's libraries and of AM2's engine that Fighters
// Megamix carries too, replaced by the toolkit's shared C++ (satrecomp/runtime/shared_lifts.h)
void RegisterShared() {
    using namespace shared;
    Register(kAm2LoadFile, 0x06029B44);
    Register(kAm2ReadPads, 0x0602BA24);
    Register(kGfsOpen, 0x06044D78);
    Register(kGfsStart, 0x06045020);
    Register(kGfsPoll, 0x06045394);
    Register(kGfsClose, 0x06045308);
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
    TheGame().frame = [] {
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
