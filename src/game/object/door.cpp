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

#include "../../common/streamutil.h"
#include "../../graphics/models.h"
#include "../../graphics/walkmeshes.h"
#include "../../resource/2da.h"
#include "../../resource/2das.h"
#include "../../resource/gffs.h"
#include "../../resource/resources.h"
#include "../../resource/strings.h"
#include "../../scene/graphs.h"
#include "../../scene/node/model.h"
#include "../../scene/types.h"
#include "../../script/scripts.h"

#include "../game.h"
#include "../services.h"

using namespace std;

using namespace reone::graphics;
using namespace reone::resource;
using namespace reone::scene;
using namespace reone::script;

namespace reone {

namespace game {

void Door::loadFromGIT(const GffStruct &gffs) {
    string templateResRef(boost::to_lower_copy(gffs.getString("TemplateResRef")));
    loadFromBlueprint(templateResRef);

    _linkedToModule = boost::to_lower_copy(gffs.getString("LinkedToModule"));
    _linkedTo = boost::to_lower_copy(gffs.getString("LinkedTo"));
    _linkedToFlags = gffs.getInt("LinkedToFlags");
    _transitionDestin = _services.strings.get(gffs.getInt("TransitionDestin"));

    loadTransformFromGIT(gffs);
}

void Door::loadFromBlueprint(const string &resRef) {
    shared_ptr<GffStruct> utd(_services.gffs.get(resRef, ResourceType::Utd));
    if (!utd) {
        return;
    }
    loadUTD(*utd);
    shared_ptr<TwoDA> doors(_services.twoDas.get("genericdoors"));
    string modelName(boost::to_lower_copy(doors->getString(_genericType, "modelname")));

    auto model = _services.models.get(modelName);
    if (!model) {
        return;
    }
    auto &sceneGraph = _services.sceneGraphs.get(_sceneName);

    auto modelSceneNode = sceneGraph.newModel(move(model), ModelUsage::Door);
    modelSceneNode->setUser(*this);
    modelSceneNode->setCullable(true);
    // modelSceneNode->setDrawDistance(_game.options().graphics.drawDistance);
    _sceneNode = move(modelSceneNode);

    auto walkmeshClosed = _services.walkmeshes.get(modelName + "0", ResourceType::Dwk);
    if (walkmeshClosed) {
        _walkmeshClosed = sceneGraph.newWalkmesh(move(walkmeshClosed));
        _walkmeshClosed->setUser(*this);
    }

    auto walkmeshOpen1 = _services.walkmeshes.get(modelName + "1", ResourceType::Dwk);
    if (walkmeshOpen1) {
        _walkmeshOpen1 = sceneGraph.newWalkmesh(move(walkmeshOpen1));
        _walkmeshOpen1->setUser(*this);
        _walkmeshOpen1->setEnabled(false);
    }

    auto walkmeshOpen2 = _services.walkmeshes.get(modelName + "2", ResourceType::Dwk);
    if (walkmeshOpen2) {
        _walkmeshOpen2 = sceneGraph.newWalkmesh(move(walkmeshOpen2));
        _walkmeshOpen2->setUser(*this);
        _walkmeshOpen2->setEnabled(false);
    }
}

void Door::loadTransformFromGIT(const GffStruct &gffs) {
    _position[0] = gffs.getFloat("X");
    _position[1] = gffs.getFloat("Y");
    _position[2] = gffs.getFloat("Z");

    _orientation = glm::quat(glm::vec3(0.0f, 0.0f, gffs.getFloat("Bearing")));

    updateTransform();
}

bool Door::isSelectable() const {
    return !_static && !_open;
}

void Door::open(const shared_ptr<Object> &triggerrer) {
    auto model = static_pointer_cast<ModelSceneNode>(_sceneNode);
    if (model) {
        // model->setDefaultAnimation("opened1", AnimationProperties::fromFlags(AnimationFlags::loop));
        model->playAnimation("opening1");
    }
    if (_walkmeshOpen1) {
        _walkmeshOpen1->setEnabled(true);
    }
    if (_walkmeshOpen2) {
        _walkmeshOpen2->setEnabled(false);
    }
    if (_walkmeshClosed) {
        _walkmeshClosed->setEnabled(false);
    }
    _open = true;
}

void Door::close(const shared_ptr<Object> &triggerrer) {
    auto model = static_pointer_cast<ModelSceneNode>(_sceneNode);
    if (model) {
        // model->setDefaultAnimation("closed", AnimationProperties::fromFlags(AnimationFlags::loop));
        model->playAnimation("closing1");
    }
    if (_walkmeshOpen1) {
        _walkmeshOpen1->setEnabled(false);
    }
    if (_walkmeshOpen2) {
        _walkmeshOpen2->setEnabled(false);
    }
    if (_walkmeshClosed) {
        _walkmeshClosed->setEnabled(true);
    }
    _open = false;
}

void Door::setLocked(bool locked) {
    _locked = locked;
}

void Door::loadUTD(const GffStruct &utd) {
    _tag = boost::to_lower_copy(utd.getString("Tag"));
    _name = _services.strings.get(utd.getInt("LocName"));
    _blueprintResRef = boost::to_lower_copy(utd.getString("TemplateResRef"));
    _autoRemoveKey = utd.getBool("AutoRemoveKey");
    _conversation = boost::to_lower_copy(utd.getString("Conversation"));
    _interruptable = utd.getBool("Interruptable");
    _faction = utd.getEnum("Faction", Faction::Invalid);
    _plot = utd.getBool("Plot");
    _minOneHP = utd.getBool("Min1HP");
    _keyRequired = utd.getBool("KeyRequired");
    _lockable = utd.getBool("Lockable");
    _locked = utd.getBool("Locked");
    _openLockDC = utd.getInt("OpenLockDC");
    _keyName = utd.getString("KeyName");
    _hitPoints = utd.getInt("HP");
    _currentHitPoints = utd.getInt("CurrentHP");
    _hardness = utd.getInt("Hardness");
    _fortitude = utd.getInt("Fort");
    _genericType = utd.getInt("GenericType");
    _static = utd.getBool("Static");

    _onClosed = utd.getString("OnClosed");   // always empty, but could be useful
    _onDamaged = utd.getString("OnDamaged"); // always empty, but could be useful
    _onDeath = utd.getString("OnDeath");
    _onHeartbeat = utd.getString("OnHeartbeat");
    _onLock = utd.getString("OnLock");                   // always empty, but could be useful
    _onMeleeAttacked = utd.getString("OnMeleeAttacked"); // always empty, but could be useful
    _onOpen = utd.getString("OnOpen");
    _onSpellCastAt = utd.getString("OnSpellCastAt"); // always empty, but could be useful
    _onUnlock = utd.getString("OnUnlock");           // always empty, but could be useful
    _onUserDefined = utd.getString("OnUserDefined");
    _onClick = utd.getString("OnClick");
    _onFailToOpen = utd.getString("OnFailToOpen");

    // Unused fields:
    //
    // - Description (always -1)
    // - CloseLockDC (always 0)
    // - PortraitId (not applicable, mostly 0)
    // - TrapDetectable (not applicable, always 1)
    // - TrapDetectDC (not applicable, always 0)
    // - TrapDisarmable (not applicable, always 1)
    // - DisarmDC (not applicable, mostly 28)
    // - TrapFlag (not applicable, always 0)
    // - TrapOneShot (not applicable, always 1)
    // - TrapType (not applicable)
    // - AnimationState (always 0)
    // - Appearance (always 0)
    // - Ref (always 0)
    // - Will (always 0)
    // - OnDisarm (not applicable, always empty)
    // - OnTrapTriggered (not applicable, always empty)
    // - LoadScreenID (always 0)
    // - PaletteID (toolset only)
    // - Comment (toolset only)
}

void Door::updateTransform() {
    Object::updateTransform();

    if (_walkmeshOpen1) {
        _walkmeshOpen1->setLocalTransform(_transform);
    }
    if (_walkmeshOpen2) {
        _walkmeshOpen2->setLocalTransform(_transform);
    }
    if (_walkmeshClosed) {
        _walkmeshClosed->setLocalTransform(_transform);
    }
}

} // namespace game

} // namespace reone
