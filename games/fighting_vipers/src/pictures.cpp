// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of Fighting Vipers Recomp: https://github.com/tmcleroy/fighting-vipers-recomp
// Fighting Vipers' pictures for the menu's Export pictures (satrecomp Game::exportPictures).
//
// The game's full-screen pictures are files of their own on the disc, each 512 by 224 dots of the Saturn's 16-bit
// colour (229,376 bytes) with the picture in the left 352: the characters' (PO_xxxnn.BIN, five places each in the
// table the ending and Options Plus's PORTRAIT show them from), the game over picture (OVER_00.BIN), and, on the
// Japanese disc only, Pepsiman's (PO_PEPnn) and the gallery of 22 more (ZOMAKE00-21, PORTRAIT's last pictures;
// japanese.cpp). Every such file is written: those of the player's disc, then those only the Japanese disc has, then
// the Japanese disc's own versions of the ones that differ (eleven: the American release changed them), named as
// the American ones with "-japan".
#include "game.h"
#include "lift.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <string>
#include <vector>

namespace satrecomp {

namespace {

constexpr size_t kPictureBytes = 512 * 224 * 2;
constexpr int kStride = 512, kWidth = 352, kHeight = 224;

// The characters' codes in the pictures' names
struct Code {
    const char *code, *name;
};
constexpr Code kCodes[] = {{"GRA", "grace"}, {"BAH", "bahn"},   {"RAX", "raxel"},    {"TKO", "tokio"},
                           {"SAN", "sanman"}, {"JAN", "jane"},  {"HON", "honey"},    {"PIC", "picky"},
                           {"BM", "bm"},      {"KUM", "kumachan"}, {"PAN", "pandachan"}, {"PEP", "pepsiman"}};

// A file's name as the picture is called: PO_GRA01.BIN → grace-1, ZOMAKE05.BIN → gallery-06, OVER_00 → game-over
std::string PictureName(const std::string &file) {
    const std::string stem = file.substr(0, file.find('.'));
    if (stem.rfind("ZOMAKE", 0) == 0) {
        const int n = std::atoi(stem.c_str() + 6) + 1;
        return std::string("gallery-") + (n < 10 ? "0" : "") + std::to_string(n);
    }
    if (stem == "OVER_00") {
        return "game-over";
    }
    if (stem.rfind("PO_", 0) == 0) {
        const std::string rest = stem.substr(3);
        for (const Code &c : kCodes) {
            const std::string code = c.code;
            if (rest.rfind(code, 0) == 0 && rest.size() > code.size() && std::isdigit(static_cast<unsigned char>(rest[code.size()]))) {
                return std::string(c.name) + "-" + std::to_string(std::atoi(rest.c_str() + code.size()));
            }
        }
    }
    std::string lower = stem;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return lower;
}

bool IsPictureFile(const std::string &name) {
    return name.rfind("PO_", 0) == 0 || name.rfind("ZOMAKE", 0) == 0 || name == "OVER_00.BIN";
}

void ExportPictures(PictureOut &out) {
    std::set<std::string> ours;
    auto give = [&](const std::string &name, const std::vector<uint8_t> &data) {
        if (data.size() == kPictureBytes) {
            out.picture(name, data.data(), kWidth, kHeight, kStride);
        }
    };
    for (const std::string &file : DiscFileNames(false)) {
        std::vector<uint8_t> data;
        if (IsPictureFile(file) && ReadDiscFile(file, data)) {
            ours.insert(file);
            give(PictureName(file), data);
        }
    }
    for (const std::string &file : DiscFileNames(true)) {
        std::vector<uint8_t> theirs, mine;
        if (!IsPictureFile(file) || !ReadExtraDiscFile(file, theirs)) {
            continue;
        }
        if (!ours.count(file)) {
            give(PictureName(file), theirs);
        } else if (ReadDiscFile(file, mine) && mine != theirs) {
            give(PictureName(file) + "-japan", theirs);
        }
    }
}

} // namespace

void RegisterPictures() {
    TheGame().exportPictures = ExportPictures;
}

} // namespace satrecomp
