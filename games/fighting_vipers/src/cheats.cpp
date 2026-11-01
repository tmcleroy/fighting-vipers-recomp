// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
// Fighting Vipers: cheats. Optional features in the category "Cheats" (the launcher's Features page and the
// in-game menu switch them). What they do is in the game's rules as rewritten in fight.cpp: where damage is
// taken and where the round's clock counts down.
#include "game.h"

namespace satrecomp {

void RegisterCheats() {
    Game &game = TheGame();
    game.features.push_back({"no_damage", "Cheats", "No damage", "Player 1's gauge does not go down."});
    game.features.push_back({"no_damage_2", "Cheats", "No damage, player 2", "Player 2's gauge does not go down (the opponent's, in arcade mode)."});
    game.features.push_back({"one_hit", "Cheats", "One-hit knockouts", "Player 2 (the opponent, in arcade mode) goes down with the first hit."});
    game.features.push_back({"infinite_time", "Cheats", "Infinite time", "The round's clock stands still."});
    game.features.push_back({"quick_rounds", "Game", "Quick rounds", "The waits before a round, after it and while the winner is shown are cut short."});
}

} // namespace satrecomp
