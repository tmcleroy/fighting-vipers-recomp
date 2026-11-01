// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
// The free camera: the fight seen from wherever the player turns it, round the two fighters.
//
// Each frame the game makes its view in one place (0x06005DEE, fv.h `camera`): the 3D library's matrix made a
// unit one (0x06052024), turned about x by the pitch and about y by the yaw, both from the camera's object, and
// moved by the eye's place (three longs in metres). The free camera puts its own pitch, yaw and eye there just
// before, so that everything drawn with that view (the fighters, the cage, the effects, the hit overlay) is seen
// from it, and gives the game its own values back as soon as the view has taken them (at the move that ends it,
// whose arguments are then in registers): the game's camera goes on from its own, and nothing of the free
// camera's is left in the game's memory.
//
// It turns round the fighters: the midpoint of their bodies (the centres of the hit task's parts) stays where the
// game's view has it in the picture, the eye goes round it by the turn the player chose, and nearer or farther.
// With no turn and no change of distance the view is the game's own.
#include "fv.h"
#include "game.h"
#include "lift.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace fv::camera {

namespace {
    constexpr float kPi = 3.14159265358979f;
    constexpr float kTurnASecond = 1.6f;  // radians a second, the stick all the way
    constexpr float kZoomASecond = 0.9f;  // the distance's factor's change a second (as a power of two)
    constexpr float kHighest = 1.2f;      // radians the free camera may go above or below the game's

    struct State {
        float yaw = 0, pitch = 0, distance = 1; // the player's turn and distance, from the game's view
        bool swapped = false;                    // this frame's view has the free camera's values in it
        uint16_t gamePitch = 0, gameYaw = 0;
        uint32_t object = 0;
        uint32_t gameEye[3] = {};
    } g;

    float Angle(uint16_t units) {
        return static_cast<float>(static_cast<int16_t>(units)) * kPi / 32768.0f;
    }
    uint16_t Units(float radians) {
        return static_cast<uint16_t>(static_cast<int32_t>(std::lround(radians * 32768.0f / kPi)));
    }
    float Metres(uint32_t fixed) {
        return static_cast<float>(static_cast<int32_t>(fixed)) / 65536.0f;
    }
    uint32_t Fixed(float metres) {
        return static_cast<uint32_t>(static_cast<int32_t>(std::lround(metres * 65536.0f)));
    }

    // The view's turn as the library makes it (0x0605280C then 0x0605289C, each multiplying the matrix on the
    // right): R = Rx(pitch) Ry(yaw), Rx = [1 0 0; 0 c -s; 0 s c], Ry = [c 0 s; 0 1 0; -s 0 c]
    void Turn(float pitch, float yaw, float r[9]) {
        const float cx = std::cos(pitch), sx = std::sin(pitch), cy = std::cos(yaw), sy = std::sin(yaw);
        const float rx[9] = {1, 0, 0, 0, cx, -sx, 0, sx, cx};
        const float ry[9] = {cy, 0, sy, 0, 1, 0, -sy, 0, cy};
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                r[i * 3 + j] = rx[i * 3] * ry[j] + rx[i * 3 + 1] * ry[3 + j] + rx[i * 3 + 2] * ry[6 + j];
            }
        }
    }
    // A place in metres as the view's world has it (the hit task's and the eye's z is the other way round)
    void World(const float metres[3], float out[3]) {
        out[0] = metres[0];
        out[1] = metres[1];
        out[2] = -metres[2];
    }

    // The fighters' midpoint, in metres: the mean of both bodies' parts
    void Midpoint(satrecomp::Guest &guest, float out[3]) {
        out[0] = out[1] = out[2] = 0;
        for (int player = 0; player < 2; player++) {
            for (uint32_t part = 0; part < hit::kParts; part++) {
                for (int axis = 0; axis < 3; axis++) {
                    out[axis] += Metres(guest.Read32(hit::kCentres[player] + part * 12 + static_cast<uint32_t>(axis) * 4));
                }
            }
        }
        for (int axis = 0; axis < 3; axis++) {
            out[axis] /= 2.0f * hit::kParts;
        }
    }

    // FV_CAMERA=YAW,PITCH,DISTANCE (degrees, degrees, a factor): the free camera turned so far from the start,
    // for tests and pictures with no window
    void FromEnvironment() {
        static bool read = false;
        if (read) {
            return;
        }
        read = true;
        if (const char *given = getenv("FV_CAMERA")) {
            float yaw = 0, pitch = 0, distance = 1;
            if (sscanf(given, "%f,%f,%f", &yaw, &pitch, &distance) >= 1) {
                g.yaw = yaw * kPi / 180.0f;
                g.pitch = pitch * kPi / 180.0f;
                g.distance = std::clamp(distance, 0.25f, 4.0f);
            }
        }
    }

    // The view's unit matrix, where the frame's view is begun (called from camera::kViewMade - 4): the free
    // camera's pitch, yaw and eye in place of the game's. The library's routine then runs as always.
    bool ViewBegun(satrecomp::Guest &guest) {
        if (guest.PR() != kViewMade || !satrecomp::FeatureOn("free_camera")) {
            return false;
        }
        FromEnvironment();
        const uint32_t object = guest.R(14);
        g.object = object;
        g.gamePitch = guest.Read16(object + kPitch);
        g.gameYaw = guest.Read16(object + kYaw);
        float eye[3];
        for (int axis = 0; axis < 3; axis++) {
            g.gameEye[axis] = guest.Read32(kEye + static_cast<uint32_t>(axis) * 4);
            eye[axis] = Metres(g.gameEye[axis]);
        }
        // (the view's angles: the pitch the library turns by is half a turn less the object's word, the yaw the
        // word the other way round)
        const float pitch = Angle(static_cast<uint16_t>(0x8000 - g.gamePitch)), yaw = Angle(static_cast<uint16_t>(-g.gameYaw));
        const float freePitch = pitch + g.pitch, freeYaw = yaw + g.yaw;

        // Where the game's view has the fighters' midpoint, from the eye: kept there, nearer or farther
        float middle[3], at[3], from[3], r[9];
        Midpoint(guest, middle);
        World(middle, at);
        World(eye, from);
        Turn(pitch, yaw, r);
        const float d[3] = {at[0] - from[0], at[1] - from[1], at[2] - from[2]};
        float seen[3];
        for (int i = 0; i < 3; i++) {
            seen[i] = (r[i * 3] * d[0] + r[i * 3 + 1] * d[1] + r[i * 3 + 2] * d[2]) * g.distance;
        }
        // The new eye: the midpoint less the same view-space offset turned back by the free camera's turn (R')
        Turn(freePitch, freeYaw, r);
        float world[3];
        for (int j = 0; j < 3; j++) {
            world[j] = at[j] - (r[j] * seen[0] + r[3 + j] * seen[1] + r[6 + j] * seen[2]);
        }
        const float freeEye[3] = {world[0], world[1], -world[2]};

        guest.Write16(object + kPitch, static_cast<uint16_t>(0x8000 - Units(freePitch)));
        guest.Write16(object + kYaw, static_cast<uint16_t>(-Units(freeYaw)));
        for (int axis = 0; axis < 3; axis++) {
            guest.Write32(kEye + static_cast<uint32_t>(axis) * 4, Fixed(freeEye[axis]));
        }
        g.swapped = true;
        return false;
    }

    // The game's own values back (the move that ends the view's making, called from camera::kViewMoved - 4, has
    // the eye in its registers already): the camera's logic, later in the frame, goes on from them
    bool ViewMoved(satrecomp::Guest &guest) {
        if (guest.PR() != kViewMoved || !g.swapped) {
            return false;
        }
        g.swapped = false;
        guest.Write16(g.object + kPitch, g.gamePitch);
        guest.Write16(g.object + kYaw, g.gameYaw);
        for (int axis = 0; axis < 3; axis++) {
            guest.Write32(kEye + static_cast<uint32_t>(axis) * 4, g.gameEye[axis]);
        }
        return false;
    }

    const satrecomp::Lift kLifts[] = {{"ViewBegun", kUnitMatrix, 0x2A, 0x7B2C65CF6A7E1B6Dull, ViewBegun},
                                      {"ViewMoved", kMove, 0x50, 0x25B449D06990D1BCull, ViewMoved}};
} // namespace

} // namespace fv::camera

namespace satrecomp {

void RegisterCamera() {
    RegisterLifts(fv::camera::kLifts, std::size(fv::camera::kLifts));
    TheGame().features.push_back({"free_camera", "Display", "Free camera",
                                  "The fight seen from wherever you turn the camera, round the two fighters: a gamepad's right "
                                  "stick, or I, J, K and L on the keyboard; U and O come nearer and go farther. It starts as the "
                                  "game's camera and follows the fight as that does. Only the picture changes: the game plays the same.",
                                  0});
}

// After every frame the game ran: the player's turn for the next frame
void CameraAfterFrame() {
    using namespace fv::camera;
    g.swapped = false;
    if (!FeatureOn("free_camera")) {
        // (switched off: the game's camera, and the next time on it starts from there)
        g.yaw = g.pitch = 0;
        g.distance = 1;
        return;
    }
    if (RunningAhead()) {
        return; // (the frames run ahead are taken back; the player's turn goes on once a frame shown)
    }
    const LookInput &look = Look();
    constexpr float kFrame = 1.0f / 59.83f;
    g.yaw += look.x * kTurnASecond * kFrame;
    g.pitch = std::clamp(g.pitch + look.y * kTurnASecond * kFrame, -kHighest, kHighest);
    g.distance = std::clamp(g.distance * std::exp2(-look.zoom * kZoomASecond * kFrame), 0.25f, 4.0f);
}

} // namespace satrecomp
