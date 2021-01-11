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

#include "item.h"

#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "../../render/textures.h"
#include "../../resource/resources.h"

#include "../object/item.h"

using namespace std;

using namespace reone::render;
using namespace reone::resource;

namespace reone {

namespace game {

ItemBlueprint::ItemBlueprint(const string &resRef, const shared_ptr<GffStruct> &uti) :
    _resRef(resRef),
    _uti(uti) {

    if (!uti) {
        throw invalid_argument("uti must not be null");
    }
}

void ItemBlueprint::load(Item &item) {
    item._blueprintResRef = _resRef;
    item._tag = boost::to_lower_copy(_uti->getString("Tag"));
    item._localizedName = Resources::instance().getString(_uti->getInt("LocalizedName"));

    shared_ptr<TwoDaTable> baseItems(Resources::instance().get2DA("baseitems"));
    int baseItem = _uti->getInt("BaseItem");

    item._equipableSlots = baseItems->getUint(baseItem, "equipableslots", 0);

    string itemClass(baseItems->getString(baseItem, "itemclass"));
    item._itemClass = boost::to_lower_copy(itemClass);

    string iconResRef;

    if (item.isEquippable(kInventorySlotBody)) {
        item._baseBodyVariation = boost::to_lower_copy(baseItems->getString(baseItem, "bodyvar"));
        item._bodyVariation = _uti->getInt("BodyVariation", 1);
        item._textureVariation = _uti->getInt("TextureVar", 1);

        iconResRef = str(boost::format("i%s_%03d") % item._itemClass % item._textureVariation);

    } else if (item.isEquippable(kInventorySlotRightWeapon)) {
        item._modelVariation = _uti->getInt("ModelVariation", 1);
        iconResRef = str(boost::format("i%s_%03d") % item._itemClass % item._modelVariation);

    } else {
        item._modelVariation = _uti->getInt("ModelVariation", 1);
        iconResRef = str(boost::format("i%s_%03d") % item._itemClass % item._modelVariation);
    }
    item._icon = Textures::instance().get(iconResRef, TextureType::GUI);

    item._attackRange = baseItems->getInt(baseItem, "maxattackrange");
    item._numDice = baseItems->getInt(baseItem, "numdice");
    item._dieToRoll = baseItems->getInt(baseItem, "dietoroll");
    item._damageFlags = baseItems->getInt(baseItem, "damageflags");
    item._weaponType = static_cast<WeaponType>(baseItems->getInt(baseItem, "weapontype"));
    item._weaponWield = static_cast<WeaponWield>(baseItems->getInt(baseItem, "weaponwield"));
}

const string &ItemBlueprint::resRef() const {
    return _resRef;
}

} // namespace game

} // namespace reone
