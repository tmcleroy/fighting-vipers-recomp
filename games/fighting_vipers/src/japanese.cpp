// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
// What the Japanese release has that the American one left out, from the player's Japanese disc (the port's
// --extra-disc; nothing of it is in this repository): Pepsiman, Honey's other clothes, and Options Plus's
// PORTRAIT (the picture gallery) and PEPSIMAN RESET.
//
// PEPSIMAN
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
//   there and only there. (The Japanese versus masks also have 0x6000, which the American game drops too;
//   its four further Honey slots, 11 to 14, have Honey's own files there and other clothes in the Japanese
//   program. Here only 0x0200 is added.)
// - On the character select he is left of Kumachan, left of Grace; in training, in the row of the hidden
//   characters. His select portrait and profile are in the American SELE.BIN already, and the training's
//   lessons (CMD_TBL.BIN) have the same rows in both releases, in English here.
// So with this feature on, the ninth character's file names are the Japanese ones and the five masks take
// 0x0200 too; the game does the rest as the Japanese one does, and he is unlocked as there (or by "Unlock
// everything", cheats.cpp). Off, the American game is as it was.
#include "fv.h"
#include "game.h"
#include "lift.h"
#include "shared_lifts.h"

#include <cstring>
#include <initializer_list>
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

// Sets an entry of the file tables to `want`, if it now holds one of `known` (else it is not the table there,
// and is left alone)
void SetName(uint32_t at, const char *want, std::initializer_list<const char *> known) {
    uint8_t *p = Ram(at);
    if (!p || std::strncmp(reinterpret_cast<const char *>(p), want, kNameSize) == 0) {
        return;
    }
    for (const char *name : known) {
        if (std::strncmp(reinterpret_cast<const char *>(p), name, kNameSize) == 0) {
            std::memset(p, 0, kNameSize);
            std::memcpy(p, want, std::strlen(want));
            return;
        }
    }
}

// After every frame: the program's tables and masks as the feature says (a state loaded, or the feature
// switched in the menu, may have left them the other way)
void SetProgram(bool on) {
    for (const Name &n : kNames) {
        SetName(n.at, on ? n.japanese : n.american, {n.american, n.japanese});
    }
    for (const Mask &m : kProgramMasks) {
        if (const uint16_t want = MaskWord(Ram16(m.at), m, on); want) {
            SetRam16(m.at, want);
        }
    }
}

// HONEY'S CLOTHES
//
// The file tables' characters 11 to 14 are Honey in other clothes in the Japanese program (and Honey as she is
// in the American one, whose disc has none of their files): models MPAI, MNHO, MEHO and MSHO (0 and 1) with
// their textures (T...), the first three on Honey's own motions, the fourth on its own (MOT_SHN) with its own
// win file (WSHONEY): Aloha Honey, who has a ukulele attack. EHO is Uniform Honey. Characters 11 to 14 are set up for them in the program (its per-character tables);
// giving Honey's own number (6) other clothes' files does not work: the model set-up (0x06022018) does not
// finish for those whose models differ. In the Japanese game the character select gives Honey's player one
// of 11 to 14 (with buttons held as she is chosen, once unlocked: bits 0x4000 and 0x2000 of the unlock word,
// set at 0x0601DBF6 when arcade mode is finished as her on the third and fourth settings, and save bytes
// 0x00200B01 and 0x002000E5); the American select program has that code cut out, and it runs from VDP1's
// memory, where nothing can be attached. So here they are a choice of the port's: with clothes chosen,
// characters 11 to 14's entries in the tables name their files, as in the Japanese program, and a player
// who has chosen Honey (0x06091EE0, a byte a player) is given that character: every Honey wears them.
constexpr const char *kHoneyFeature = "honey_clothes";
// Characters 11 to 14's entries in the tables, as the Japanese program has them (the American: Honey's)
struct Clothes {
    const char *model0, *model1, *texture0, *texture1, *motion, *win;
};
constexpr Clothes kHoney = {"MHON0.BIN", "MHON1.BIN", "THON0.BIN", "THON1.BIN", "MOT_HON.BIN", "WHONEY.BIN"};
constexpr Clothes kClothes[4] = {
    {"MPAI0.BIN", "MPAI1.BIN", "TPAI0.BIN", "TPAI1.BIN", "MOT_HON.BIN", "WHONEY.BIN"},
    {"MNHO0.BIN", "MNHO1.BIN", "TNHO0.BIN", "TNHO1.BIN", "MOT_HON.BIN", "WHONEY.BIN"},
    {"MEHO0.BIN", "MEHO1.BIN", "TEHO0.BIN", "TEHO1.BIN", "MOT_HON.BIN", "WHONEY.BIN"},
    {"MSHO0.BIN", "MSHO1.BIN", "TSHO0.BIN", "TSHO1.BIN", "MOT_SHN.BIN", "WSHONEY.BIN"},
};
constexpr int kHoneyCharacter = 6, kFirstClothes = 11;
// Where each table's entries start (character 0; the motions' table has a first entry of its own)
constexpr uint32_t kModels0 = 0x06085CAC, kModels1 = 0x06085DEC, kTextures0 = 0x06085F2C, kTextures1 = 0x0608606C,
                   kMotions = 0x060861AC + kNameSize, kWins = 0x06086580;
// The characters the players have chosen (bytes, player 1 and 2; round.cpp reads them for the records)
constexpr uint32_t kChosen = 0x06091EE0;

int g_haveClothes = -1;

// Which clothes (1-4), 0: her own
int HoneyClothes() {
    const int chosen = FeatureValue(kHoneyFeature);
    if (chosen <= 0 || chosen > 4) {
        return 0;
    }
    if (g_haveClothes < 0) {
        std::vector<uint8_t> data;
        g_haveClothes = ReadDiscFile("MPAI0.BIN", data) ? 1 : 0;
        if (!g_haveClothes) {
            fprintf(stderr, "Honey's clothes: not on the disc (the Japanese disc is needed too); left as they are\n");
        }
    }
    return g_haveClothes == 1 ? chosen : 0;
}

// The table of pictures (0x060867B0: five a character, 20 bytes a name; the arcade's ending and the Portrait
// viewer show them) has Pandachan's at character 11 and Tokio's (Pepsiman's with him in) at 12, and Honey's at 13
// and 14. While Honey wears other clothes in arcade mode, 11 and 12 show Honey's own pictures; they are put back
// as they were after, so that the viewer has them as before.
constexpr uint32_t kPictures = 0x060867B0, kPicturesPerCharacter = 5;
std::vector<std::string> g_picturesKept; // (11's and 12's names while they show Honey's)

void SetHoneyPictures(bool honeys) {
    auto at = [](uint32_t character, uint32_t n) { return kPictures + (character * kPicturesPerCharacter + n) * kNameSize; };
    auto name = [](uint32_t address) {
        const uint8_t *p = Ram(address);
        return p ? std::string(reinterpret_cast<const char *>(p), strnlen(reinterpret_cast<const char *>(p), kNameSize)) : std::string();
    };
    auto put = [](uint32_t address, const std::string &text) {
        if (uint8_t *p = Ram(address); p && text.size() < kNameSize) {
            std::memset(p, 0, kNameSize);
            std::memcpy(p, text.data(), text.size());
        }
    };
    if (honeys && g_picturesKept.empty()) {
        for (uint32_t character : {11u, 12u}) {
            for (uint32_t n = 0; n < kPicturesPerCharacter; n++) {
                g_picturesKept.push_back(name(at(character, n)));
                put(at(character, n), name(at(kHoneyCharacter, n)));
            }
        }
    } else if (!honeys && !g_picturesKept.empty()) {
        size_t i = 0;
        for (uint32_t character : {11u, 12u}) {
            for (uint32_t n = 0; n < kPicturesPerCharacter; n++) {
                put(at(character, n), g_picturesKept[i++]);
            }
        }
        g_picturesKept.clear();
    }
}

void SetHoneyClothes(int chosen) {
    for (int i = 0; i < 4; i++) {
        const Clothes &c = chosen ? kClothes[i] : kHoney;
        const Clothes &other = chosen ? kHoney : kClothes[i];
        const uint32_t at = static_cast<uint32_t>(kFirstClothes + i) * kNameSize;
        SetName(kModels0 + at, c.model0, {other.model0});
        SetName(kModels1 + at, c.model1, {other.model1});
        SetName(kTextures0 + at, c.texture0, {other.texture0});
        SetName(kTextures1 + at, c.texture1, {other.texture1});
        SetName(kMotions + at, c.motion, {other.motion});
        SetName(kWins + at, c.win, {other.win});
    }
    // (the Japanese game has them in versus, team battle and training; here in arcade too, where the ending
    // shows the pictures of the character's place in the table of pictures: Honey's own, see SetHoneyPictures)
    const uint8_t mode = Ram(fv::kGameMode) ? *Ram(fv::kGameMode) : 0;
    SetHoneyPictures(chosen != 0 && mode == fv::kModeArcade);
    if (mode != fv::kModeArcade && mode != fv::kModeVersus && mode != fv::kModeTeam && mode != fv::kModeTraining) {
        return;
    }
    // Only a Honey a person has chosen. In arcade and team battle the game's own fighters are written to the same
    // bytes, in the same frame as it sets them up: one of them changed then is Honey's model on another
    // character's tables (with Aloha's, a ball that jumps about the arena). Versus and training choose both on
    // the character select, before anything is set up.
    const uint8_t people = Ram(fv::kPlayers) ? *Ram(fv::kPlayers) : 0;
    const bool againstGame = mode == fv::kModeArcade || mode == fv::kModeTeam;
    for (uint32_t player = 0; player < 2; player++) {
        if (againstGame && (people & (1u << player)) == 0) {
            continue;
        }
        uint8_t *p = Ram(kChosen + player);
        if (p && chosen && *p == kHoneyCharacter) {
            *p = static_cast<uint8_t>(kFirstClothes - 1 + chosen);
        }
        // (switched off, nothing is put back: a Honey made one of 11 to 14 is Honey as the American program
        // has those, with her own files)
    }
}

// OPTIONS PLUS: PORTRAIT AND PEPSIMAN RESET
//
// Options Plus (main states 50 and 51: 0x06032F14, 0x06033044) has eight rows, each a place and a label in a
// table (0x06087B24: x, y, the label's address). Row 3 is PEPSIMAN RESET (locks him again: clears 0x0200 of the
// unlock word) and row 4 PORTRAIT (the picture viewer, main states 37 to 39: the characters' pictures and the
// gallery's ZOMAKE00-21, which only the Japanese disc has). The American program has both rows whole, labels
// and what they do, and leaves them out in three places only: the cursor skips them (0x060330CA-0x060330E4:
// cmp/eq #3 and #4), the drawing skips them (0x060332F2, 0x060332F6) and its table of each row's drawing sends
// both to the end (0x06033352, 0x06033354: 0x66 where the others have 0x3E). In the Japanese program the cursor
// skips neither, PORTRAIT is drawn as the others and PEPSIMAN RESET only once he is unlocked. Here the compares
// are given -1 (no row) and the table entries the others' value: PORTRAIT with the feature "portrait" (and the
// Japanese disc), PEPSIMAN RESET with Pepsiman in and unlocked.
constexpr const char *kPortraitFeature = "portrait";
struct CodeWord {
    uint32_t at;
    uint16_t american, shown;
};
constexpr uint16_t kCmpEqMinus1 = 0x88FF;
constexpr CodeWord kPortraitWords[] = {
    {0x060330CE, 0x8804, kCmpEqMinus1}, {0x060330D8, 0x8804, kCmpEqMinus1}, // the cursor
    {0x060332F6, 0x8804, kCmpEqMinus1},                                   // the drawing
    {0x06033354, 0x0066, 0x003E},                                         // its table
};
constexpr CodeWord kResetWords[] = {
    {0x060330CA, 0x8803, kCmpEqMinus1}, {0x060330E4, 0x8803, kCmpEqMinus1},
    {0x060332F2, 0x8803, kCmpEqMinus1},
    {0x06033352, 0x0066, 0x003E},
};
constexpr uint32_t kUnlocks = 0x00200080;

int g_haveGallery = -1;

bool PortraitOn() {
    if (!FeatureOn(kPortraitFeature)) {
        return false;
    }
    if (g_haveGallery < 0) {
        std::vector<uint8_t> data;
        g_haveGallery = ReadDiscFile("ZOMAKE00.BIN", data) ? 1 : 0;
        if (!g_haveGallery) {
            fprintf(stderr, "Portrait: the gallery's pictures are not on the disc (the Japanese disc is needed too); left out\n");
        }
    }
    return g_haveGallery == 1;
}

template <size_t N>
void SetCode(const CodeWord (&words)[N], bool shown) {
    for (const CodeWord &w : words) {
        const uint16_t now = Ram16(w.at);
        if (now != w.american && now != w.shown) {
            continue; // (not the game's code there)
        }
        if (const uint16_t want = shown ? w.shown : w.american; now != want) {
            SetRam16(w.at, want);
        }
    }
}

void SetOptionsPlus(bool pepsiman) {
    SetCode(kPortraitWords, PortraitOn());
    SetCode(kResetWords, pepsiman && (Ram16(kUnlocks) & kPepsiman) != 0);
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

void JapaneseAfterFrame() {
    const bool pepsiman = On();
    SetProgram(pepsiman);
    SetOptionsPlus(pepsiman);
    SetHoneyClothes(HoneyClothes());
}

void RegisterJapanese() {
    TheGame().features.push_back({kFeature, "Game", "Pepsiman",
                                  "The Japanese release's extra character, from your Japanese disc (keep it in the same folder as "
                                  "your American disc). Unlocked as in that release, or by Unlock everything; on the character "
                                  "select, move left from Grace. From the next character select.", kFeatureEnhanced});
    TheGame().features.push_back({kHoneyFeature, "Game", "Honey's clothes",
                                  "The other clothes the Japanese release gives Honey (Candy), from your Japanese disc (keep it in "
                                  "the same folder as your American disc). Every Honey chosen wears them, in arcade, versus, team "
                                  "battle and training. From her next fight.",
                                  0, {"Her own", "Pink", "White ribbon", "Uniform", "Aloha (with a ukulele attack)"}});
    TheGame().features.push_back({kPortraitFeature, "Game", "Portrait in Options Plus",
                                  "The Japanese release's picture viewer, back in Options Plus: the characters' pictures and the "
                                  "gallery's 22 more, from your Japanese disc (keep it in the same folder as your American disc).",
                                  kFeatureEnhanced});
    const shared::Routine &original = shared::kAm2LoadFile;
    const shared::Routine loadFile = {original.name, original.size, original.hash, LoadFile};
    shared::Register(loadFile, 0x06029B44);
}

} // namespace satrecomp
