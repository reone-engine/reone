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

#pragma once

#include <string>
#include <unordered_set>

#include "../../resource/format/2dafile.h"

#include "../types.h"

#include "attributes.h"
#include "savingthrows.h"

namespace reone {

namespace game {

class CreatureClass {
public:
    CreatureClass(ClassType type);

    void load(const resource::TwoDA &twoDa, int row);

    bool isClassSkill(Skill skill) const;

    /**
     * Calculates a defense bonus that creature of this class and the specified
     * level would have.
     */
    int getDefenseBonus(int level) const;

    /**
     * @return class saving throws at the specified creature level
     */
    const SavingThrows &getSavingThrows(int level) const;

    const std::string &name() const { return _name; }
    const std::string &description() const { return _description; }
    int hitdie() const { return _hitdie; }
    const CreatureAttributes &defaultAttributes() const { return _defaultAttributes; }
    int skillPointBase() const { return _skillPointBase; }

private:
    ClassType _type;
    std::string _name;
    std::string _description;
    int _hitdie { 0 };
    CreatureAttributes _defaultAttributes;
    int _skillPointBase { 0 };
    std::unordered_set<Skill> _classSkills;
    std::unordered_map<int, SavingThrows> _savingThrowsByLevel;

    void loadClassSkills(const std::string &skillsTable);
    void loadSavingThrows(const std::string &savingThrowTable);
};

} // namespace game

} // namespace reone
