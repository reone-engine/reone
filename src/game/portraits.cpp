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

#include "portraits.h"

#include "../../graphics/textures.h"
#include "../../resource/2da.h"
#include "../../resource/2das.h"

using namespace std;

using namespace reone::graphics;
using namespace reone::resource;

namespace reone {

namespace game {

void Portraits::init() {
    shared_ptr<TwoDA> portraits(_twoDas.get("portraits"));
    if (!portraits) {
        return;
    }

    for (int row = 0; row < portraits->getRowCount(); ++row) {
        string resRef(boost::to_lower_copy(portraits->getString(row, "baseresref")));

        Portrait portrait;
        portrait.resRef = resRef;
        portrait.appearanceNumber = portraits->getInt(row, "appearancenumber");
        portrait.appearanceS = portraits->getInt(row, "appearance_s");
        portrait.appearanceL = portraits->getInt(row, "appearance_l");
        portrait.forPC = portraits->getBool(row, "forpc");
        portrait.sex = portraits->getInt(row, "sex");

        _portraits.push_back(move(portrait));
    }
}

shared_ptr<Texture> Portraits::getTextureByIndex(int index) {
    shared_ptr<Texture> result;
    if (index >= 0 && index < static_cast<int>(_portraits.size())) {
        result = getPortraitTexture(_portraits[index]);
    }
    return move(result);
}

shared_ptr<Texture> Portraits::getPortraitTexture(const Portrait &portrait) const {
    return _textures.get(portrait.resRef, TextureUsage::GUI);
}

shared_ptr<Texture> Portraits::getTextureByAppearance(int appearance) {
    for (auto &portrait : _portraits) {
        if (portrait.appearanceNumber == appearance ||
            portrait.appearanceS == appearance ||
            portrait.appearanceL == appearance)
            return getPortraitTexture(portrait);
    }
    return nullptr;
}

} // namespace game

} // namespace reone
