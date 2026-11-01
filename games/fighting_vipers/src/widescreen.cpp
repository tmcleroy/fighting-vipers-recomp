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
// sides. In a wider picture the cage's sides would end there, so the limit goes out by as much as the
// picture does. It is one word next to each routine's code, used for both directions; down, panels that
// far off the picture are then listed too, which shows only when the camera is right at the cage.
#include "game.h"

#include <algorithm>
#include <cmath>

namespace satrecomp {

// After every frame the game ran (the fight's code is loaded with the fight; a state loaded may have been
// saved with another picture's limit)
void WidescreenAfterFrame() {
    constexpr uint32_t kLimits[] = {0x06022D8E, 0x06022E9A};
    constexpr uint16_t kLimit = 224, kHalfWidth = 176;
    const float wider = PictureAspect() / (4.0f / 3.0f) - 1.0f;
    const uint16_t limit = static_cast<uint16_t>(kLimit + std::lround(std::max(wider, 0.0f) * kHalfWidth));
    for (const uint32_t at : kLimits) {
        // (mov.w @(limit),r1; bra; neg r1,r2: the routine is there)
        if (Ram16(at - 6) == 0x9101 && Ram16(at - 2) == 0x621B && Ram16(at) != limit) {
            SetRam16(at, limit);
        }
    }
}

void RegisterWidescreen() {
    constexpr uint32_t kNBG2 = 2;
    TheGame().wideSplits = {
        // rows of the 448-line picture; columns: where the left player's gauge ends and the right one's begins
        {kNBG2, 0, 112, 312, 392},
        // the stage number and the play time (messages in between are up to 240 dots wide)
        {kNBG2, 392, 448, 232, 472},
    };
}

} // namespace satrecomp
