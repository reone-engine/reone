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

#include <boost/algorithm/string.hpp>

#include "../../common/log.h"

#include "../blueprint/blueprints.h"
#include "../enginetype/location.h"
#include "../game.h"

using namespace std;

using namespace reone::script;

namespace reone {

namespace game {

Variable Routines::destroyObject(const VariablesList &args, ExecutionContext &ctx) {
    auto destroy = getSpatialObject(args, 0);
    if (destroy) {
        _game->module()->area()->destroyObject(*destroy);
    } else {
        warn("Routines: destroyObject: destroy is invalid");
    }
    return Variable();
}

Variable Routines::getEnteringObject(const VariablesList &args, ExecutionContext &ctx) {
    return static_pointer_cast<ScriptObject>(getTriggerrer(ctx));
}

Variable Routines::getIsObjectValid(const VariablesList &args, ExecutionContext &ctx) {
    return static_cast<bool>(getObject(args, 0));
}

Variable Routines::getObjectByTag(const VariablesList &args, ExecutionContext &ctx) {
    string tag(getString(args, 0));
    boost::to_lower(tag);

    // Apparently, empty string in this context stands for the player
    if (tag.empty()) {
        return static_pointer_cast<ScriptObject>(_game->party().player());
    }
    int nth = getInt(args, 1, 0);

    return static_pointer_cast<ScriptObject>(_game->module()->area()->getObjectByTag(tag, nth));
}

Variable Routines::getWaypointByTag(const VariablesList &args, ExecutionContext &ctx) {
    string tag(getString(args, 0));
    boost::to_lower(tag);

    for (auto &waypoint : _game->module()->area()->getObjectsByType(ObjectType::Waypoint)) {
        if (waypoint->tag() == tag) {
            return static_pointer_cast<ScriptObject>(waypoint);
        }
    }

    return shared_ptr<ScriptObject>();
}

Variable Routines::getArea(const VariablesList &args, ExecutionContext &ctx) {
    return static_pointer_cast<ScriptObject>(_game->module()->area());
}

Variable Routines::setLocked(const VariablesList &args, ExecutionContext &ctx) {
    auto target = getDoor(args, 0);
    if (target) {
        bool locked = getBool(args, 1);
        target->setLocked(locked);
    } else {
        warn("Routines: setLocked: target is invalid");
    }
    return Variable();
}

Variable Routines::getLocked(const VariablesList &args, ExecutionContext &ctx) {
    auto target = getDoor(args, 0);
    if (!target) {
        warn("Routines: getLocked: target is invalid");
        return 0;
    }
    return target->isLocked() ? 1 : 0;
}

Variable Routines::getModule(const VariablesList &args, ExecutionContext &ctx) {
    return static_pointer_cast<ScriptObject>(_game->module());
}

Variable Routines::getTag(const VariablesList &args, ExecutionContext &ctx) {
    static string empty;

    auto object = getObject(args, 0);
    if (!object) {
        warn("Routines: getTag: object is invalid");
        return empty;
    }

    return object->tag();
}

Variable Routines::getDistanceToObject(const VariablesList &args, ExecutionContext &ctx) {
    auto caller = getCallerAsSpatial(ctx);
    if (!caller) {
        warn("Routines: getDistanceToObject: caller is invalid");
        return -1.0f;
    }
    auto object = getSpatialObject(args, 0);
    if (!object) {
        warn("Routines: getDistanceToObject: object is invalid");
        return -1.0f;
    }

    return caller->distanceTo(*object);
}

Variable Routines::getDistanceToObject2D(const VariablesList &args, ExecutionContext &ctx) {
    auto caller = getCallerAsSpatial(ctx);
    if (!caller) {
        warn("Routines: getDistanceToObject2D: caller is invalid");
        return -1.0f;
    }
    auto object = getSpatialObject(args, 0);
    if (!object) {
        warn("Routines: getDistanceToObject2D: object is invalid");
        return -1.0f;
    }

    return caller->distanceTo(glm::vec2(object->position()));
}

Variable Routines::getExitingObject(const VariablesList &args, ExecutionContext &ctx) {
    return static_pointer_cast<ScriptObject>(getTriggerrer(ctx));
}

Variable Routines::getFacing(const VariablesList &args, ExecutionContext &ctx) {
    auto target = getSpatialObject(args, 0);
    if (!target) {
        warn("Routines: getFacing: target is invalid");
        return -1.0f;
    }
    return glm::degrees(target->facing());
}

Variable Routines::getPosition(const VariablesList &args, ExecutionContext &ctx) {
    auto target = getSpatialObject(args, 0);
    if (!target) {
        warn("Routines: getPosition: target is invalid");
        return glm::vec3(0.0f);
    }
    return target->position();
}

Variable Routines::soundObjectPlay(const VariablesList &args, ExecutionContext &ctx) {
    auto sound = getSound(args, 0);
    if (sound) {
        sound->play();
    } else {
        warn("Routines: soundObjectPlay: sound is invalid");
    }
    return Variable();
}

Variable Routines::soundObjectStop(const VariablesList &args, ExecutionContext &ctx) {
    auto sound = getSound(args, 0);
    if (sound) {
        sound->stop();
    } else {
        warn("Routines: soundObjectStop: sound is invalid");
    }
    return Variable();
}

Variable Routines::getDistanceBetween(const VariablesList &args, ExecutionContext &ctx) {
    auto objectA = getSpatialObject(args, 0);
    if (!objectA) {
        warn("Routines: getDistanceBetween: objectA is invalid");
        return -1.0f;
    }
    auto objectB = getSpatialObject(args, 1);
    if (!objectB) {
        warn("Routines: getDistanceBetween: objectB is invalid");
        return -1.0f;
    }

    return objectA->distanceTo(*objectB);
}

Variable Routines::getDistanceBetween2D(const VariablesList &args, ExecutionContext &ctx) {
    auto objectA = getSpatialObject(args, 0);
    if (!objectA) {
        warn("Routines: getDistanceBetween2D: objectA is invalid");
        return 0.0f;
    }
    auto objectB = getSpatialObject(args, 1);
    if (!objectB) {
        warn("Routines: getDistanceBetween2D: objectB is invalid");
        return 0.0f;
    }
    return objectA->distanceTo(glm::vec2(objectB->position()));
}

Variable Routines::getIsDead(const VariablesList &args, ExecutionContext &ctx) {
    auto creature = getCreature(args, 0);
    if (!creature) {
        warn("Routines: getIsDead: creature is invalid");
        return false;
    }
    return creature->isDead();
}

Variable Routines::getIsInCombat(const VariablesList &args, ExecutionContext &ctx) {
    auto creature = getCreatureOrCaller(args, 0, ctx);
    if (!creature) {
        warn("Routines: getIsInCombat: creature is invalid");
        return false;
    }
    return creature->isInCombat();
}

Variable Routines::getIsOpen(const VariablesList &args, ExecutionContext &ctx) {
    auto object = getSpatialObject(args, 0);
    if (!object) {
        warn("Routines: getIsOpen: object is invalid");
        return false;
    }
    return object->isOpen();
}

Variable Routines::setFacing(const VariablesList &args, ExecutionContext &ctx) {
    auto caller = getCallerAsSpatial(ctx);
    if (caller) {
        float direction = getFloat(args, 0);
        caller->setFacing(glm::radians(direction));
    } else {
        warn("Routines: setFacing: caller is invalid");
    }
    return Variable();
}

Variable Routines::setFacingPoint(const VariablesList &args, ExecutionContext &ctx) {
    auto caller = getCallerAsSpatial(ctx);
    if (caller) {
        glm::vec3 target(getVector(args, 0));
        caller->face(target);
    } else {
        warn("Routines: setFacingPoint: caller is invalid");
    }
    return Variable();
}

Variable Routines::getName(const VariablesList &args, ExecutionContext &ctx) {
    static string empty;

    auto object = getObject(args, 0);
    if (!object) {
        warn("Routines: getName: object is invalid");
        return empty;
    }

    return object->name();
}

Variable Routines::getObjectType(const VariablesList &args, ExecutionContext &ctx) {
    auto target = getObject(args, 0);
    if (!target) {
        warn("Routines: getObjectType: target is invalid");
        return static_cast<int>(ObjectType::Invalid);
    }
    return static_cast<int>(target->type());
}

Variable Routines::getPlotFlag(const VariablesList &args, ExecutionContext &ctx) {
    auto target = getObjectOrCaller(args, 0, ctx);
    if (!target) {
        warn("Routines: getPlotFlag: target is invalid");
        return 0;
    }
    return target->plotFlag();
}

Variable Routines::setPlotFlag(const VariablesList &args, ExecutionContext &ctx) {
    auto target = getObject(args, 0);
    if (target) {
        int plotFlag = getInt(args, 1);
        target->setPlotFlag(plotFlag);
    } else {
        warn("Routines: setPlotFlag: target is invalid");
    }
    return Variable();
}

Variable Routines::faceObjectAwayFromObject(const VariablesList &args, ExecutionContext &ctx) {
    auto facer = getSpatialObject(args, 0);
    if (!facer) {
        warn("Routines: faceObjectAwayFromObject: facer is invalid");
        return Variable();
    }
    auto objectToFaceAwayFrom = getSpatialObject(args, 1);
    if (!objectToFaceAwayFrom) {
        warn("Routines: faceObjectAwayFromObject: objectToFaceAwayFrom is invalid");
        return Variable();
    }
    facer->faceAwayFrom(*objectToFaceAwayFrom);

    return Variable();
}

Variable Routines::getCommandable(const VariablesList &args, ExecutionContext &ctx) {
    auto target = getObjectOrCaller(args, 0, ctx);
    if (!target) {
        warn("Routines: getCommandable: target is invalid");
        return 0;
    }
    return target->isCommandable() ? 1 : 0;
}

Variable Routines::setCommandable(const VariablesList &args, ExecutionContext &ctx) {
    auto target = getObjectOrCaller(args, 1, ctx);
    if (target) {
        bool commandable = getBool(args, 0);
        target->setCommandable(commandable);
    } else {
        warn("Routines: setCommandable: target is invalid");
    }
    return Variable();
}

Variable Routines::playAnimation(const VariablesList &args, ExecutionContext &ctx) {
    auto caller = getCallerAsSpatial(ctx);
    if (caller) {
        AnimationType animation = static_cast<AnimationType>(getInt(args, 0));
        float speed = getFloat(args, 1, 1.0f);
        float seconds = getFloat(args, 2, 0.0f); // TODO: handle duration
        caller->playAnimation(animation, speed);
    } else {
        warn("Routines: playAnimation: caller is invalid");
    }
    return Variable();
}

Variable Routines::getLastOpenedBy(const VariablesList &args, ExecutionContext &ctx) {
    return static_pointer_cast<ScriptObject>(getTriggerrer(ctx));
}

Variable Routines::getAreaUnescapable(const VariablesList &args, ExecutionContext &ctx) {
    return _game->module()->area()->isUnescapable() ? 1 : 0;
}

Variable Routines::setAreaUnescapable(const VariablesList &args, ExecutionContext &ctx) {
    bool unescapable = getBool(args, 0);
    _game->module()->area()->setUnescapable(unescapable);
    return Variable();
}

Variable Routines::cutsceneAttack(const VariablesList &args, ExecutionContext &ctx) {
    auto caller = getCallerAsCreature(ctx);
    if (!caller) {
        warn("Routines: cutsceneAttack: caller is invalid");
        return Variable();
    }
    auto target = getSpatialObject(args, 0);
    if (!target) {
        warn("Routines: cutsceneAttack: target is invalid");
        return Variable();
    }
    int animation = getInt(args, 1);
    AttackResultType attackResult = static_cast<AttackResultType>(getInt(args, 2));
    int damage = getInt(args, 3);

    Combat &combat = _game->module()->area()->combat();
    combat.cutsceneAttack(caller, target, animation, attackResult, damage);

    return Variable();
}

Variable Routines::createObject(const VariablesList &args, ExecutionContext &ctx) {
    auto objectType = static_cast<ObjectType>(getInt(args, 0));
    string blueprintResRef(boost::to_lower_copy(getString(args, 1)));
    auto location = getLocationEngineType(args, 2);
    bool useAppearAnimation = getBool(args, 3, false);

    return static_pointer_cast<ScriptObject>(_game->module()->area()->createObject(objectType, blueprintResRef, location));
}

Variable Routines::getNearestCreature(const VariablesList &args, ExecutionContext &ctx) {
    int firstCriteriaType = getInt(args, 0);
    int firstCriteriaValue = getInt(args, 1);
    auto target = getSpatialObjectOrCaller(args, 2, ctx);
    int nth = getInt(args, 3, 1);
    int secondCriteriaType = getInt(args, 4, -1);
    int secondCriteriaValue = getInt(args, 5, -1);
    int thirdCriteriaType = getInt(args, 6, -1);
    int thirdCriteriaValue = getInt(args, 7, -1);

    // TODO: handle criterias
    shared_ptr<SpatialObject> object(_game->module()->area()->getNearestObject(target->position(), nth - 1, [](auto &object) {
        return object->type() == ObjectType::Creature;
    }));

    return static_pointer_cast<ScriptObject>(object);
}

Variable Routines::getNearestCreatureToLocation(const VariablesList &args, ExecutionContext &ctx) {
    int firstCriteriaType = getInt(args, 0);
    int firstCriteriaValue = getInt(args, 1);
    auto location = getLocationEngineType(args, 2);
    int nth = getInt(args, 3, 1);
    int secondCriteriaType = getInt(args, 4, -1);
    int secondCriteriaValue = getInt(args, 5, -1);
    int thirdCriteriaType = getInt(args, 6, -1);
    int thirdCriteriaValue = getInt(args, 7, -1);

    // TODO: handle criterias
    shared_ptr<SpatialObject> object(_game->module()->area()->getNearestObject(location->position(), nth - 1, [](auto &object) {
        return object->type() == ObjectType::Creature;
    }));

    return static_pointer_cast<ScriptObject>(object);
}

Variable Routines::getNearestObject(const VariablesList &args, ExecutionContext &ctx) {
    auto objectType = static_cast<ObjectType>(getInt(args, 0, static_cast<int>(ObjectType::All)));
    auto target = getSpatialObjectOrCaller(args, 1, ctx);
    int nth = getInt(args, 2, 1);

    shared_ptr<SpatialObject> object(_game->module()->area()->getNearestObject(target->position(), nth - 1, [&objectType](auto &object) {
        return object->type() == objectType;
    }));

    return static_pointer_cast<ScriptObject>(object);
}

Variable Routines::getNearestObjectToLocation(const VariablesList &args, ExecutionContext &ctx) {
    auto objectType = static_cast<ObjectType>(getInt(args, 0));
    auto location = getLocationEngineType(args, 1);
    int nth = getInt(args, 2, 1);

    shared_ptr<SpatialObject> object(_game->module()->area()->getNearestObject(location->position(), nth - 1, [&objectType](auto &object) {
        return object->type() == objectType;
    }));

    return static_pointer_cast<ScriptObject>(object);
}

Variable Routines::getNearestObjectByTag(const VariablesList &args, ExecutionContext &ctx) {
    string tag(boost::to_lower_copy(getString(args, 0)));
    auto target = getSpatialObjectOrCaller(args, 1, ctx);
    int nth = getInt(args, 2, 1);

    shared_ptr<SpatialObject> object(_game->module()->area()->getNearestObject(target->position(), nth - 1, [&tag](auto &object) {
        return object->tag() == tag;
    }));

    return static_pointer_cast<ScriptObject>(object);
}

} // namespace game

} // namespace reone
