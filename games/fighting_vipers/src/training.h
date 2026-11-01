// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
// Fighting Vipers: training aids, built on the rewritten fight rules (fight.cpp calls these).
#pragma once

#include "lift.h"

#include <cstdint>

namespace fv::training {

// A fighter (0 or 1) has taken damage off their gauge
void Damage(int player, int32_t damage);
// Where a player's input for the frame is taken (fightinput.cpp), with what their pad gives: the input the
// fighter is to have instead (the dummy's; nothing, for the player while the dummy is recorded), or -1
int Input(satrecomp::Guest &g, int player, uint8_t fromPad);
// A fighter's input for the frame, as it goes into the history (fightinput.cpp)
void InputSeen(int player, uint8_t input, bool onTheRight);
// A frame of the fight (paused: nothing of it moved on)
void Frame(satrecomp::Guest &g, bool paused);

} // namespace fv::training
