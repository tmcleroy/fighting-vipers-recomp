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
    // Unlock everything: what the game unlocks is in its save file FVIPER_MAST (3,088 bytes, read to 0x00200000
    // and kept there), and the toolkit shows the game that file with these fields raised, keeping the player's
    // own (Game::saveUnlocks). Found from the code that reads and sets them:
    // - 0x00200080, a word of unlocks: 0x0100 when arcade mode is first finished (0x0601DB5E: Mahler, and the
    //   title's Options Plus), 0x8000 when finished on the hardest setting (B.M.), 0x0400 at the 50th match
    //   (0x0601D300: Kumachan). The screens read it through 0x85FF or 0x05FF; 0x0200 is the Japanese disc's
    //   Pepsiman, left alone.
    // - 0x00200048, the matches played: Big Heads in Options Plus from the 200th (0x0603336E, "more than 199").
    // - 0x00200AF0, training mode's OKs: the wall display from the 301st (0x060336B4, "more than 299").
    game.features.push_back({"unlock_all", "Cheats", "Unlock everything",
                             "Mahler, B.M., Kumachan, Options Plus, Big Heads and the wall display, without playing for them. Your own "
                             "save is kept as it is: turned off, the game has what you have unlocked. From the next start or reset."});
    game.saveUnlocks = {"unlock_all", "FVIPER_MAST", {
        {0x80, 2, SaveField::kBits, 0x8500},
        {0x48, 4, SaveField::kAtLeast, 200},
        {0xAF0, 2, SaveField::kAtLeast, 301},
    }};
    game.features.push_back({"quick_rounds", "Game", "Quick rounds", "The waits before a round, after it and while the winner is shown are cut short."});
}

} // namespace satrecomp
