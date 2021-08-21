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

/** @file
 *  Encounter functions related to blueprint loading.
 */

#include "encounter.h"

#include "../../resource/strings.h"

#include "../game.h"

using namespace std;

using namespace reone::resource;

namespace reone {

namespace game {

void Encounter::loadUTE(const GffStruct &ute) {
    _tag = boost::to_lower_copy(ute.getString("Tag"));
    _name = _game->services().resource().strings().get(ute.getInt("LocalizedName"));
    _blueprintResRef = boost::to_lower_copy(ute.getString("TemplateResRef"));
    _active = ute.getBool("Active");
    _difficultyIndex = ute.getInt("DifficultyIndex"); // index into encdifficulty.2da
    _faction = ute.getEnum("Faction", Faction::Invalid);
    _maxCreatures = ute.getInt("MaxCreatures");
    _playerOnly = ute.getBool("PlayerOnly");
    _reset = ute.getBool("Reset");
    _resetTime = ute.getInt("ResetTime");
    _respawns = ute.getInt("Respawns");

    _onEntered = ute.getString("OnEntered");
    _onExit = ute.getString("OnExit"); // always empty, but could be useful
    _onExhausted = ute.getString("OnExhausted"); // always empty, but could be useful
    _onHeartbeat = ute.getString("OnHeartbeat"); // always empty, but could be useful
    _onUserDefined = ute.getString("OnUserDefined"); // always empty, but could be useful

    loadCreaturesFromUTE(ute);

    // Unused fields:
    //
    // - Difficulty (obsolete)
    // - SpawnOption (always 1)
    // - PaletteID (toolset only)
    // - Comment (toolset only)
}

void Encounter::loadCreaturesFromUTE(const GffStruct &ute) {
    for (auto &creatureGffs : ute.getList("CreatureList")) {
        EncounterCreature creature;
        creature._appearance = creatureGffs->getInt("Appearance");
        creature._cr = creatureGffs->getFloat("CR");
        creature._resRef = creatureGffs->getString("ResRef");
        creature._singleSpawn = creatureGffs->getBool("SingleSpawn");
        _creatures.push_back(move(creature));
    }
}

} // namespace game

} // namespace reone
