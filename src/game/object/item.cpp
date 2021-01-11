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

using namespace std;

using namespace reone::render;
using namespace reone::resource;

namespace reone {

namespace game {

Item::Item(uint32_t id) : Object(id, ObjectType::Item) {
}

void Item::load(const shared_ptr<ItemBlueprint> &blueprint) {
    if (!blueprint) {
        throw invalid_argument("blueprint must not be null");
    }
    blueprint->load(*this);
}

bool Item::isEquippable() const {
    return _equipableSlots != 0;
}

bool Item::isEquippable(InventorySlot slot) const {
    return (_equipableSlots >> slot) & 1;
}

const string &Item::localizedName() const {
    return _localizedName;
}

const string &Item::baseBodyVariation() const {
    return _baseBodyVariation;
}

int Item::bodyVariation() const {
    return _bodyVariation;
}

int Item::textureVariation() const {
    return _textureVariation;
}

const string &Item::itemClass() const {
    return _itemClass;
}

int Item::modelVariation() const {
    return _modelVariation;
}

shared_ptr<Texture> Item::icon() const {
    return _icon;
}

float Item::attackRange() const {
    return static_cast<float>(_attackRange);
}

int Item::numDice() const {
    return _numDice;
}

int Item::dieToRoll() const {
    return _dieToRoll;
}

int Item::damageFlags() const {
    return _damageFlags;
}

WeaponType Item::weaponType() const {
    return _weaponType;
}

WeaponWield Item::weaponWield() const {
    return _weaponWield;
}

int Item::stackSize() const {
    return _stackSize;
}

bool Item::isDropable() const {
    return _dropable;
}

void Item::setDropable(bool dropable) {
    _dropable = dropable;
}

void Item::setStackSize(int stackSize) {
    _stackSize = stackSize;
}

bool Item::isIdentified() const {
    return _identified;
}

void Item::setIdentified(bool value) {
    _identified = value;
}

bool Item::isEquipped() const {
    return _equipped;
}

void Item::setEquipped(bool equipped) {
    _equipped = equipped;
}

} // namespace game

} // namespace reone
