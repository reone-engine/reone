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

#include "routines.h"

#include "../../common/log.h"

#include "../game.h"
#include "../object/creature.h"
#include "../rp/factionutil.h"

using namespace std;

using namespace reone::script;

namespace reone {

namespace game {

Variable Routines::getGender(const VariablesList &args, ExecutionContext &ctx) {
    auto creature = getCreature(args, 0);
    if (!creature) {
        warn("Routines: getGender: creature is invalid");
        return static_cast<int>(Gender::None);
    }
    return static_cast<int>(creature->gender());
}

Variable Routines::getHitDice(const VariablesList &args, ExecutionContext &ctx) {
    auto creature = getCreature(args, 0);
    if (!creature) {
        warn("Routines: getGender: creature is invalid");
        return static_cast<int>(Gender::None);
    }
    return static_cast<int>(creature->attributes().getAggregateLevel());
}

Variable Routines::getClassByPosition(const VariablesList &args, ExecutionContext &ctx) {
    auto creature = getCreatureOrCaller(args, 1, ctx);
    if (!creature) {
        warn("Routines: getClassByPosition: creature is invalid");
        return static_cast<int>(ClassType::Invalid);
    }
    int position = getInt(args, 0);
    return static_cast<int>(creature->attributes().getClassByPosition(position));
}

Variable Routines::getLevelByClass(const VariablesList &args, ExecutionContext &ctx) {
    auto creature = getCreatureOrCaller(args, 1, ctx);
    if (!creature) {
        warn("Routines: getLevelByClass: creature is invalid");
        return 0;
    }
    ClassType clazz = static_cast<ClassType>(getInt(args, 0));
    return creature->attributes().getClassLevel(clazz);
}

Variable Routines::getHasSkill(const VariablesList &args, ExecutionContext &ctx) {
    auto creature = getCreatureOrCaller(args, 1, ctx);
    if (!creature) {
        warn("Routines: getHasSkill: creature is invalid");
        return 0;
    }
    Skill skill = static_cast<Skill>(getInt(args, 0));
    return creature->attributes().skills().contains(skill) ? 1 : 0;
}

Variable Routines::getCurrentHitPoints(const VariablesList &args, ExecutionContext &ctx) {
    auto object = getObjectOrCaller(args, 0, ctx);
    if (!object) {
        warn("Routines: getCurrentHitPoints: object is invalid");
        return 0;
    }
    return object->currentHitPoints();
}

Variable Routines::getMaxHitPoints(const VariablesList &args, ExecutionContext &ctx) {
    auto object = getObjectOrCaller(args, 0, ctx);
    if (!object) {
        warn("Routines: getMaxHitPoints: object is invalid");
        return 0;
    }
    return object->maxHitPoints();
}

Variable Routines::getMinOneHP(const VariablesList &args, ExecutionContext &ctx) {
    auto object = getObject(args, 0);
    if (!object) {
        warn("Routines: getMinOneHP: object is invalid");
        return 0;
    }
    return object->isMinOneHP() ? 1 : 0;
}

Variable Routines::setMaxHitPoints(const VariablesList &args, ExecutionContext &ctx) {
    auto object = getObject(args, 0);
    if (!object) {
        warn("Routines: setMaxHitPoints: object is invalid");
        return 0;
    }
    int maxHP = getInt(args, 1);
    object->setMaxHitPoints(maxHP);

    return Variable();
}

Variable Routines::setMinOneHP(const VariablesList &args, ExecutionContext &ctx) {
    auto object = getObject(args, 0);
    if (!object) {
        warn("Routines: setMinOneHP: object is invalid");
        return 0;
    }
    bool minOneHP = getBool(args, 1);
    object->setMinOneHP(minOneHP);

    return Variable();
}

Variable Routines::changeFaction(const VariablesList &args, ExecutionContext &ctx) {
    auto objectToChangeFaction = getCreature(args, 0);
    if (!objectToChangeFaction) {
        warn("Routines: changeFaction: objectToChangeFaction is invalid");
        return Variable();
    }
    auto memberOfFactionToJoin = getCreature(args, 1);
    if (!memberOfFactionToJoin) {
        warn("Routines: changeFaction: memberOfFactionToJoin is invalid");
        return Variable();
    }
    objectToChangeFaction->setFaction(memberOfFactionToJoin->faction());

    return Variable();
}

Variable Routines::changeToStandardFaction(const VariablesList &args, ExecutionContext &ctx) {
    auto creatureToChange = getCreature(args, 0);
    if (creatureToChange) {
        Faction faction = static_cast<Faction>(getInt(args, 1));
        creatureToChange->setFaction(faction);
    } else {
        warn("Routines: changeToStandardFaction: creatureToChange is invalid");
    }
    return Variable();
}

Variable Routines::getFactionEqual(const VariablesList &args, ExecutionContext &ctx) {
    auto firstObject = getCreature(args, 0);
    if (!firstObject) {
        warn("Routines: getStandardFaction: firstObject is invalid");
        return 0;
    }
    auto secondObject = getCreatureOrCaller(args, 1, ctx);
    if (!secondObject) {
        warn("Routines: getStandardFaction: secondObject is invalid");
        return 0;
    }
    return firstObject->faction() == secondObject->faction() ? 1 : 0;
}

Variable Routines::getStandardFaction(const VariablesList &args, ExecutionContext &ctx) {
    auto object = getCreature(args, 0);
    if (!object) {
        warn("Routines: getStandardFaction: object is invalid");
        return static_cast<int>(Faction::Invalid);
    }
    return static_cast<int>(object->faction());
}

Variable Routines::getIsEnemy(const VariablesList &args, ExecutionContext &ctx) {
    auto target = getCreature(args, 0);
    if (!target) {
        warn("Routines: getIsEnemy: target is invalid");
        return 0;
    }
    auto source = getCreatureOrCaller(args, 1, ctx);
    if (!source) {
        warn("Routines: getIsEnemy: source is invalid");
        return 0;
    }
    return game::getIsEnemy(*target, *source);
}

Variable Routines::getIsFriend(const VariablesList &args, ExecutionContext &ctx) {
    auto target = getCreature(args, 0);
    if (!target) {
        warn("Routines: getIsFriend: target is invalid");
        return 0;
    }
    auto source = getCreatureOrCaller(args, 1, ctx);
    if (!source) {
        warn("Routines: getIsFriend: source is invalid");
        return 0;
    }
    return game::getIsFriend(*target, *source);
}

Variable Routines::getIsNeutral(const VariablesList &args, ExecutionContext &ctx) {
    auto target = getCreature(args, 0);
    if (!target) {
        warn("Routines: getIsNeutral: target is invalid");
        return 0;
    }
    auto source = getCreatureOrCaller(args, 1, ctx);
    if (!source) {
        warn("Routines: getIsNeutral: source is invalid");
        return 0;
    }
    return game::getIsNeutral(*target, *source);
}

Variable Routines::getAbilityScore(const VariablesList &args, ExecutionContext &ctx) {
    auto creature = getCreature(args, 0);
    if (!creature) {
        warn("Routines: getAbilityScore: creature is invalid");
        return 0;
    }
    Ability type = static_cast<Ability>(getInt(args, 1));

    return creature->attributes().abilities().getScore(type);
}

Variable Routines::getLevelByPosition(const VariablesList &args, ExecutionContext &ctx) {
    auto creature = getCreatureOrCaller(args, 1, ctx);
    if (!creature) {
        warn("Routines: getLevelByPosition: creature is invalid");
        return 0;
    }
    int position = getInt(args, 0);

    return creature->attributes().getLevelByPosition(position);
}

Variable Routines::getSkillRank(const VariablesList &args, ExecutionContext &ctx) {
    auto object = getCreatureOrCaller(args, 1, ctx);
    if (!object) {
        warn("Routines: getSkillRank: object is invalid");
        return 0;
    }
    Skill skill = static_cast<Skill>(getInt(args, 0));

    return object->attributes().skills().getRank(skill);
}

Variable Routines::getXP(const VariablesList &args, ExecutionContext &ctx) {
    auto creature = getCreature(args, 0);
    if (!creature) {
        warn("Routines: getXP: creature is invalid");
        return 0;
    }
    return creature->xp();
}

Variable Routines::setXP(const VariablesList &args, ExecutionContext &ctx) {
    auto creature = getCreature(args, 0);
    if (creature) {
        int xpAmount = getInt(args, 1);
        creature->setXP(xpAmount);
    } else {
        warn("Routines: setXP: creature is invalid");
    }
    return Variable();
}

Variable Routines::giveXPToCreature(const VariablesList &args, ExecutionContext &ctx) {
    auto creature = getCreature(args, 0);
    if (creature) {
        int xpAmount = getInt(args, 1);
        creature->giveXP(xpAmount);
    } else {
        warn("Routines: giveXPToCreature: creature is invalid");
    }
    return Variable();
}

Variable Routines::getMaxStealthXP(const VariablesList &args, ExecutionContext &ctx) {
    return _game->module()->area()->maxStealthXP();
}

Variable Routines::setMaxStealthXP(const VariablesList &args, ExecutionContext &ctx) {
    int max = getInt(args, 0);
    _game->module()->area()->setMaxStealthXP(max);
    return Variable();
}

Variable Routines::getCurrentStealthXP(const VariablesList &args, ExecutionContext &ctx) {
    return _game->module()->area()->currentStealthXP();
}

Variable Routines::setCurrentStealthXP(const VariablesList &args, ExecutionContext &ctx) {
    int current = getInt(args, 0);
    _game->module()->area()->setCurrentStealthXP(current);
    return Variable();
}

Variable Routines::getStealthXPEnabled(const VariablesList &args, ExecutionContext &ctx) {
    bool enabled = _game->module()->area()->isStealthXPEnabled();
    return enabled ? 1 : 0;
}

Variable Routines::setStealthXPEnabled(const VariablesList &args, ExecutionContext &ctx) {
    bool enabled = getBool(args, 0);
    _game->module()->area()->setStealthXPEnabled(enabled);
    return Variable();
}

Variable Routines::getStealthXPDecrement(const VariablesList &args, ExecutionContext &ctx) {
    return _game->module()->area()->stealthXPDecrement();
}

Variable Routines::setStealthXPDecrement(const VariablesList &args, ExecutionContext &ctx) {
    int decrement = getInt(args, 0);
    _game->module()->area()->setStealthXPDecrement(decrement);
    return Variable();
}

} // namespace game

} // namespace reone
