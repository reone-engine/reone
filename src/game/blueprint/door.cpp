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

#include "door.h"

#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include "../../resource/resources.h"

#include "../object/door.h"

using namespace std;

using namespace reone::resource;

namespace reone {

namespace game {

DoorBlueprint::DoorBlueprint(const string &resRef, const shared_ptr<GffStruct> &utd) :
    _resRef(resRef),
    _utd(utd) {

    if (!utd) {
        throw invalid_argument("utd must not be null");
    }
}

void DoorBlueprint::load(Door &door) {
    door._blueprintResRef = _resRef;
    door._tag = boost::to_lower_copy(_utd->getString("Tag"));

    int locNameStrRef = _utd->getInt("LocName", -1);
    if (locNameStrRef != -1) {
        door._name = Resources::instance().getString(locNameStrRef);
    }

    door._conversation = boost::to_lower_copy(_utd->getString("Conversation"));
    door._lockable = _utd->getBool("Lockable");
    door._locked = _utd->getBool("Locked");
    door._genericType = _utd->getInt("GenericType");
    door._static = _utd->getBool("Static");
    door._keyRequired = _utd->getBool("KeyRequired");
    door._minOneHP = _utd->getBool("Min1HP");
    door._hitPoints = _utd->getInt("HP");
    door._currentHitPoints = _utd->getInt("CurrentHP");

    loadScripts(door);
}

void DoorBlueprint::loadScripts(Door &door) {
    door._heartbeat = _utd->getString("OnHeartbeat");
    door._onOpen = _utd->getString("OnOpen");
    door._onFailToOpen = _utd->getString("OnFailToOpen");
}

} // namespace game

} // namespace reone
