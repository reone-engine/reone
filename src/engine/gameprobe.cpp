/*
 * Copyright (c) 2020-2021 The reone project contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gameprobe.h"

#include "../common/pathutil.h"

using namespace std;

using namespace reone::game;

namespace fs = boost::filesystem;

namespace reone {

namespace game {

GameID GameProbe::invoke() {
    // If there is a KotOR executable then game is KotOR
    fs::path exePathK1(getPathIgnoreCase(_gamePath, "swkotor.exe", false));
    if (!exePathK1.empty()) {
        return GameID::KotOR;
    }

    // If there is a TSL executable then game is TSL
    fs::path exePathK2(getPathIgnoreCase(_gamePath, "swkotor2.exe", false));
    if (!exePathK2.empty()) {
        return GameID::TSL;
    }

    throw logic_error("Unable to determine game ID: " + _gamePath.string());
}

} // namespace game

} // namespace reone
