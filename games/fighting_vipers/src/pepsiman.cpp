// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
// Pepsiman, the Japanese release's extra character, in the American game: his files come from the player's
// Japanese disc (the port's --extra-disc), nothing of his is in this repository.
//
// What the two releases have, found by comparing them (the American MK-81041 V2.000, the Japanese GS-9101
// V1.013):
// - The program is the same game. Its tables of the files each character is loaded from have a place for
//   seventeen characters; the ninth is Pepsiman's in both, but in the American program its names are
//   Tokio's (MTOK0.BIN where the Japanese has MPEP0.BIN, and so on). The American disc has none of his files.
// - What the game unlocks is a word of the save file at 0x00200080 (cheats.cpp); 0x0200 is Pepsiman, set
//   and cleared by the same code in both (0x0601D2E4, 0x060331DA). Five places read the word for the
//   screens that list who can be chosen: three in the program, two in the character select's own program
//   (SELEPROG.BIN, loaded into VDP1's memory at 0x05C78000 and run there). The American ones take the
//   word with 0x05FF or 0x85FF, the Japanese with 0x07FF or 0xE7FF: the American game leaves Pepsiman out
//   there and only there. (The Japanese masks also have 0x6000 for Honey's other clothes and not 0x8000,
//   B.M., whom that release had not yet; here only 0x0200 is added.)
// So with this feature on, the ninth character's file names are the Japanese ones and the five masks take
// 0x0200 too; the game does the rest as the Japanese one does, and he is unlocked as there (or by "Unlock
// everything", cheats.cpp). Off, the American game is as it was.
#include "game.h"
#include "lift.h"
#include "shared_lifts.h"

#include <cstring>
#include <string>
#include <vector>

namespace satrecomp {

namespace {

constexpr const char *kFeature = "pepsiman";

// The ninth character's entries in the file tables (20 bytes each), and their second set of portraits
// (the twelfth character's, Pepsiman's in the Japanese program as well)
struct Name {
    uint32_t at;
    const char *american, *japanese;
};
constexpr Name kNames[] = {
    {0x06085D60, "MTOK0.BIN", "MPEP0.BIN"},       {0x06085EA0, "MTOK1.BIN", "MPEP1.BIN"},
    {0x06085FE0, "TTOK0.BIN", "TPEP0.BIN"},       {0x06086120, "TTOK1.BIN", "TPEP1.BIN"},
    {0x06086274, "MOT_TKO.BIN", "MOT_PEP.BIN"},   {0x06086490, "CTOKIO.BIN", "CPEPSI.BIN"},
    {0x06086634, "WTOKIO.BIN", "WPEP.BIN"},
    {0x06086B34, "PO_TKO01.BIN", "PO_PEP01.BIN"}, {0x06086B48, "PO_TKO03.BIN", "PO_PEP02.BIN"},
    {0x06086B5C, "PO_TKO04.BIN", "PO_PEP03.BIN"}, {0x06086B70, "PO_TKO02.BIN", "PO_PEP04.BIN"},
    {0x06086B84, "PO_TKO05.BIN", "PO_PEP04.BIN"},
    {0x06086C60, "PO_TKO01.BIN", "PO_PEP01.BIN"}, {0x06086C74, "PO_TKO03.BIN", "PO_PEP02.BIN"},
    {0x06086C88, "PO_TKO04.BIN", "PO_PEP03.BIN"}, {0x06086C9C, "PO_TKO02.BIN", "PO_PEP04.BIN"},
    {0x06086CB0, "PO_TKO05.BIN", "PO_PEP04.BIN"},
};
constexpr uint32_t kNameSize = 20;

constexpr uint16_t kPepsiman = 0x0200;
struct Mask {
    uint32_t at;     // the word a mov.w loads
    uint16_t american;
};
// In the program: the records screens and the like (0x060381BE, 0x06040154, 0x06041E6A)
constexpr Mask kProgramMasks[] = {{0x060381F6, 0x85FF}, {0x060401A6, 0x85FF}, {0x06041F4C, 0x05FF}};
// In the character select's program, when it has been loaded (0x05C79E38: the select's start; the second is
// for versus play)
constexpr uint32_t kSelectProgram = 0x05C78000;
constexpr Mask kSelectMasks[] = {{kSelectProgram + 0x1F5C, 0x05FF}, {kSelectProgram + 0x1F5E, 0x85FF}};

int g_haveFiles = -1; // the Japanese disc's files can be read (-1: not looked yet)

bool On() {
    if (!FeatureOn(kFeature)) {
        return false;
    }
    if (g_haveFiles < 0) {
        std::vector<uint8_t> data;
        g_haveFiles = ReadDiscFile("MPEP0.BIN", data) ? 1 : 0;
        if (!g_haveFiles) {
            fprintf(stderr, "Pepsiman: his files are not on the disc (the Japanese disc is needed too); left out\n");
        }
    }
    return g_haveFiles == 1;
}

// A mask word as it should be, if it is one of the two it can be (else 0: not the game's code there)
uint16_t MaskWord(uint16_t now, const Mask &m, bool on) {
    if (now != m.american && now != (m.american | kPepsiman)) {
        return 0;
    }
    return on ? static_cast<uint16_t>(m.american | kPepsiman) : m.american;
}

// After every frame: the program's tables and masks as the feature says (a state loaded, or the feature
// switched in the menu, may have left them the other way)
void SetProgram(bool on) {
    for (const Name &n : kNames) {
        uint8_t *p = Ram(n.at);
        if (!p) {
            continue;
        }
        const char *want = on ? n.japanese : n.american;
        const char *other = on ? n.american : n.japanese;
        if (std::strncmp(reinterpret_cast<const char *>(p), want, kNameSize) == 0 ||
            std::strncmp(reinterpret_cast<const char *>(p), other, kNameSize) != 0) {
            continue; // (as wanted, or not the table: left alone)
        }
        std::memset(p, 0, kNameSize);
        std::memcpy(p, want, std::strlen(want));
    }
    for (const Mask &m : kProgramMasks) {
        if (const uint16_t want = MaskWord(Ram16(m.at), m, on); want) {
            SetRam16(m.at, want);
        }
    }
}

// The game's LoadFile (shared::kAm2LoadFile), and then, when it has just loaded the character select's
// program, that program's masks
bool LoadFile(Guest &g) {
    const std::string name = g.String(g.R(4), 32);
    const uint32_t destination = g.R(5);
    if (!shared::kAm2LoadFile.fn(g)) {
        return false;
    }
    if (name == "SELEPROG.BIN" && destination == kSelectProgram && On()) {
        for (const Mask &m : kSelectMasks) {
            if (const uint16_t want = MaskWord(g.Read16(m.at), m, true); want) {
                g.Write16(m.at, want);
            }
        }
    }
    return true;
}

} // namespace

void PepsimanAfterFrame() {
    SetProgram(On());
}

void RegisterPepsiman() {
    TheGame().features.push_back({kFeature, "Game", "Pepsiman",
                                  "The Japanese release's extra character, from your Japanese disc (chosen on the launcher's "
                                  "Game page). He is unlocked as in that release, or by Unlock everything. From the next "
                                  "character select."});
    const shared::Routine &original = shared::kAm2LoadFile;
    const shared::Routine loadFile = {original.name, original.size, original.hash, LoadFile};
    shared::Register(loadFile, 0x06029B44);
}

} // namespace satrecomp
