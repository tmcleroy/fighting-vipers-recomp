// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
// Fighting Vipers in widescreen: what of the game's screens belongs at the picture's edges.
//
// The 3D view needs nothing (the game does not cull at the screen's sides; satrecomp/docs/VIDEO.md). The
// fight's display is all on one scroll screen, NBG2, 704 dots wide: at the top each player's figure, name,
// gauge and round markers on either side of the timer; at the bottom the stage on the left and the play time
// on the right, with messages ("1P PAUSE") between. The two sides move out to the picture's edges; the timer
// and the messages stay in the middle.
//
// The cage is the one thing the game leaves out by where it is on the screen: the routines that list its
// panels (0x06022D1C on the master CPU, 0x06022E40 on the slave) take a panel only if all four of its
// corners are less than 224 dots from the picture's middle, across and down: 48 past the 4:3 picture's
// sides. In a wider picture the cage's sides would end there, so the limit goes out (the toolkit's
// shared::WidenAm2PanelLimits, which Fighters Megamix uses too). It is one word next to each routine's code,
// used for both directions; down, panels that far off the picture are then listed too, which shows only
// when the camera is right at the cage.
#include "fv.h"
#include "game.h"
#include "shared_lifts.h"

namespace satrecomp {

// After every frame the game ran (the fight's code is loaded with the fight; a state loaded may have been
// saved with another picture's limit)
void WidescreenAfterFrame() {
    // (the loads of the limit: mov.w @(limit),r1; bra; neg r1,r2)
    shared::WidenAm2PanelLimits({0x06022D88, 0x06022E94});
}

// The splits are the fight's display's, and are made only while it is up: in a fight (main state 8), and not in
// training mode, whose display is laid out otherwise (DAMAGE, a scale across the top, TRAINING MODE in the middle:
// the splits cut through them) and whose start has SELECT CPU TYPE on the same rows. Elsewhere NBG2's rows hold
// other screens (the character select's top row of faces, menus), which the splits cut apart and opened a gap in.
bool FightDisplayUp() {
    constexpr uint32_t kMainState = 0x06090760;
    constexpr uint32_t kMainFight = 8;
    const uint8_t *mode = Ram(fv::kGameMode);
    return Ram32(kMainState) == kMainFight && !(mode && *mode == fv::kModeTraining);
}

// The staff roll (main states 13 and 14) is laid out for 4:3: each stage's scene shows in a frame with black
// borders across the top and bottom, the names over it. Beyond the frame's sides the scroll screens hold what the
// game never shows (from the sixth stage, rows of stripes), so the picture keeps to 4:3 there.
bool StaffRoll() {
    constexpr uint32_t kMainState = 0x06090760;
    const uint32_t state = Ram32(kMainState);
    return state == 13 || state == 14;
}

void RegisterWidescreen() {
    TheGame().wideSplitsOn = FightDisplayUp;
    TheGame().pillarbox = StaffRoll;
    constexpr uint32_t kNBG2 = 2;
    TheGame().wideSplits = {
        // rows of the 448-line picture; columns: where the left player's gauge ends and the right one's begins
        {kNBG2, 0, 112, 312, 392},
        // the stage number and the play time (messages in between are up to 240 dots wide)
        {kNBG2, 392, 448, 232, 472},
    };
}

} // namespace satrecomp
