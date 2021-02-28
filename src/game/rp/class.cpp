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

#include "class.h"

#include <boost/algorithm/string.hpp>

#include "../../resource/resources.h"

using namespace std;

using namespace reone::resource;

namespace reone {

namespace game {

static const char kSkillsTwoDaResRef[] = "skills";

CreatureClass::CreatureClass(ClassType type) : _type(type) {
    _defaultAttributes.addClassLevels(type, 1);
}

void CreatureClass::load(const TwoDA &twoDa, int row) {
    _name = Resources::instance().getString(twoDa.getInt(row, "name"));
    _description = Resources::instance().getString(twoDa.getInt(row, "description"));
    _hitdie = twoDa.getInt(row, "hitdie");
    _skillPointBase = twoDa.getInt(row, "skillpointbase");

    CreatureAbilities &abilities = _defaultAttributes.abilities();
    abilities.setScore(Ability::Strength, twoDa.getInt(row, "str"));
    abilities.setScore(Ability::Dexterity, twoDa.getInt(row, "dex"));
    abilities.setScore(Ability::Constitution, twoDa.getInt(row, "con"));
    abilities.setScore(Ability::Intelligence, twoDa.getInt(row, "int"));
    abilities.setScore(Ability::Wisdom, twoDa.getInt(row, "wis"));
    abilities.setScore(Ability::Charisma, twoDa.getInt(row, "cha"));

    string skillsTable(boost::to_lower_copy(twoDa.getString(row, "skillstable")));
    loadClassSkills(skillsTable);

    string savingThrowTable(boost::to_lower_copy(twoDa.getString(row, "savingthrowtable")));
    loadSavingThrows(savingThrowTable);
}

void CreatureClass::loadClassSkills(const string &skillsTable) {
    shared_ptr<TwoDA> skills(Resources::instance().get2DA(kSkillsTwoDaResRef));
    for (int row = 0; row < skills->getRowCount(); ++row) {
        if (skills->getInt(row, skillsTable + "_class") == 1) {
            _classSkills.insert(static_cast<Skill>(row));
        }
    }
}

void CreatureClass::loadSavingThrows(const string &twoDaResRef) {
    shared_ptr<TwoDA> twoDa(Resources::instance().get2DA(twoDaResRef));
    for (int row = 0; row < twoDa->getRowCount(); ++row) {
        int level = twoDa->getInt(row, "level");

        SavingThrows throws;
        throws.fortitude = twoDa->getInt(row, "fortsave");
        throws.reflex = twoDa->getInt(row, "refsave");
        throws.will = twoDa->getInt(row, "willsave");

        _savingThrowsByLevel.insert(make_pair(level, move(throws)));
    }
}

bool CreatureClass::isClassSkill(Skill skill) const {
    return _classSkills.count(skill) > 0;
}

int CreatureClass::getDefenseBonus(int level) const {
    switch (_type) {
        case ClassType::JediConsular:
        case ClassType::JediGuardian:
        case ClassType::JediSentinel:
        case ClassType::Scoundrel:
            return 2 + (2 * (level / 6));
        default:
            return 0;
    }
}

const SavingThrows &CreatureClass::getSavingThrows(int level) const {
    auto maybeThrows = _savingThrowsByLevel.find(level);
    if (maybeThrows == _savingThrowsByLevel.end()) {
        throw logic_error("Saving throws not found for level " + to_string(level));
    }
    return maybeThrows->second;
}

} // namespace game

} // namespace reone
