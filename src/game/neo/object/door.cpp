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

#include "../../../common/exception/validation.h"
#include "../../../graphics/models.h"
#include "../../../graphics/services.h"
#include "../../../graphics/walkmeshes.h"
#include "../../../resource/2das.h"
#include "../../../resource/gffs.h"
#include "../../../resource/gff.h"
#include "../../../resource/services.h"
#include "../../../scene/graph.h"
#include "../../../scene/node/model.h"

#include "../../services.h"

using namespace std;

using namespace reone::graphics;
using namespace reone::resource;
using namespace reone::scene;

namespace reone {

namespace game {

namespace neo {

void Door::loadFromGit(const Gff &git) {
    // From GIT

    auto templateResRef = git.getString("TemplateResRef");
    auto tag = git.getString("Tag");
    auto x = git.getFloat("X");
    auto y = git.getFloat("Y");
    auto z = git.getFloat("Z");
    auto bearing = git.getFloat("Bearing");

    // From UTD

    auto utd = _resourceSvc.gffs.get(templateResRef, ResourceType::Utd);
    if (!utd) {
        throw ValidationException("UTD not found: " + templateResRef);
    }
    auto genericType = utd->getInt("GenericType");

    // From doortypes 2DA

    auto genericDoorsTable = _resourceSvc.twoDas.get("genericdoors");
    if (!genericDoorsTable) {
        throw ValidationException("genericdoors 2DA not found");
    }
    auto modelName = genericDoorsTable->getString(genericType, "modelname");

    // Make scene nodes

    shared_ptr<ModelSceneNode> sceneNode;
    auto model = _graphicsSvc.models.get(modelName);
    if (model) {
        sceneNode = _sceneGraph->newModel(move(model), ModelUsage::Door, nullptr);
        sceneNode->setUser(*this);
        sceneNode->setCullable(true);
        sceneNode->setPickable(true);
    }

    shared_ptr<WalkmeshSceneNode> walkmeshClosedSceneNode;
    auto walkmeshClosed = _graphicsSvc.walkmeshes.get(modelName + "0", ResourceType::Dwk);
    if (walkmeshClosed) {
        walkmeshClosedSceneNode = _sceneGraph->newWalkmesh(move(walkmeshClosed));
        walkmeshClosedSceneNode->setUser(*this);
    }

    shared_ptr<WalkmeshSceneNode> walkmeshOpen1SceneNode;
    auto walkmeshOpen1 = _graphicsSvc.walkmeshes.get(modelName + "1", ResourceType::Dwk);
    if (walkmeshOpen1) {
        walkmeshOpen1SceneNode = _sceneGraph->newWalkmesh(move(walkmeshOpen1));
        walkmeshOpen1SceneNode->setUser(*this);
        walkmeshOpen1SceneNode->setEnabled(false);
    }

    shared_ptr<WalkmeshSceneNode> walkmeshOpen2SceneNode;
    auto walkmeshOpen2 = _graphicsSvc.walkmeshes.get(modelName + "2", ResourceType::Dwk);
    if (walkmeshOpen2) {
        walkmeshOpen2SceneNode = _sceneGraph->newWalkmesh(move(walkmeshOpen2));
        walkmeshOpen2SceneNode->setEnabled(false);
    }

    //

    _tag = move(tag);
    _position = glm::vec3(x, y, z);
    _facing = bearing;
    _sceneNode = move(sceneNode);
    _walkmeshClosed = move(walkmeshClosedSceneNode);
    _walkmeshOpen1 = move(walkmeshOpen1SceneNode);
    _walkmeshOpen2 = move(walkmeshOpen2SceneNode);

    flushTransform();
}

void Door::handleClick(Object &clicker) {
}

void Door::flushTransform() {
    Object::flushTransform();

    auto transform = glm::translate(_position);
    transform *= glm::rotate(_facing, glm::vec3(0.0f, 0.0f, 1.0f));
    transform *= glm::rotate(_pitch, glm::vec3(1.0f, 0.0f, 0.0f));

    if (_walkmeshClosed) {
        _walkmeshClosed->setLocalTransform(transform);
    }
    if (_walkmeshOpen1) {
        _walkmeshOpen1->setLocalTransform(transform);
    }
    if (_walkmeshOpen2) {
        _walkmeshOpen2->setLocalTransform(transform);
    }
}

} // namespace neo

} // namespace game

} // namespace reone