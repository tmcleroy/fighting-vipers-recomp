// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Tommy McLeroy. Part of satrecomp: https://github.com/tmcleroy/satrecomp
// Export pictures: the game's pictures (Game::exportPictures) written as files into a folder of the player's, for
// the in-game menu's rows of that name.
#pragma once

#include <string>

namespace port {

// The folder they go to: "pictures" in the folder settings.ini is in (the player's folder, as the launcher's
// YOUR FILES shows it), or in the current folder when the game was started without settings
std::string PicturesFolder();
// Writes them all; the number written, or -1 with `error` said
int ExportPictures(const std::string &folder, std::string &error);

} // namespace port
