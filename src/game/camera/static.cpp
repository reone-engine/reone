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

#include "static.h"

#include "../../scene/graph.h"

#include "../object/placeablecamera.h"

using namespace std;

using namespace reone::graphics;
using namespace reone::scene;

namespace reone {

namespace game {

StaticCamera::StaticCamera(float aspect, SceneGraph &sceneGraph) :
    _aspect(aspect) {

    _sceneNode = sceneGraph.newCamera(glm::mat4(1.0f));
}

void StaticCamera::setObject(const PlaceableCamera &object) {
    glm::mat4 projection(glm::perspective(glm::radians(object.fieldOfView()), _aspect, kDefaultClipPlaneNear, kDefaultClipPlaneFar));
    _sceneNode->setLocalTransform(object.transform());
    _sceneNode->setProjection(projection);
}

} // namespace game

} // namespace reone
