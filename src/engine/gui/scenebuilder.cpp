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

#include "scenebuilder.h"

#include "../scene/node/camera.h"
#include "../scene/node/mesh.h"
#include "../scene/node/model.h"
#include "../scene/scenegraph.h"

using namespace std;

using namespace reone::di;
using namespace reone::graphics;
using namespace reone::scene;

namespace reone {

namespace gui {

SceneBuilder::SceneBuilder(GraphicsOptions options, GraphicsServices &graphics) :
    _options(options),
    _graphics(graphics) {

    _aspect = options.width / static_cast<float>(options.height);
}

unique_ptr<SceneGraph> SceneBuilder::build() {
    auto scene = make_unique<SceneGraph>(_options, _graphics);

    shared_ptr<ModelSceneNode> model(_modelSupplier(*scene));
    if (!model) {
        throw logic_error("model is null");
    }

    glm::mat4 projection(glm::ortho(
        -_aspect * _modelScale + _modelOffset.x,
        _aspect * _modelScale + _modelOffset.x,
        -_modelScale + _modelOffset.y,
        _modelScale + _modelOffset.y,
        _zNear, _zFar));

    scene->addRoot(model);
    scene->setAmbientLightColor(_ambientLightColor);

    auto cameraNode = make_shared<CameraSceneNode>("", projection, scene.get());
    if (_cameraNodeName.empty()) {
        cameraNode->setLocalTransform(_cameraTransform);
    } else {
        shared_ptr<ModelNode> modelNode(model->model()->getNodeByName(_cameraNodeName));
        if (modelNode) {
            cameraNode->setLocalTransform(modelNode->absoluteTransform() * _cameraTransform);
        }
    }
    scene->setActiveCamera(move(cameraNode));

    if (!_lightingRefNodeName.empty()) {
        shared_ptr<ModelNode> modelNode(model->model()->getNodeByName(_lightingRefNodeName));
        if (modelNode) {
            auto lightingRefNode = make_shared<DummySceneNode>(modelNode, scene.get());
            lightingRefNode->setLocalTransform(modelNode->absoluteTransform());
            scene->setLightingRefNode(move(lightingRefNode));
        }
    }

    return move(scene);
}

SceneBuilder &SceneBuilder::aspect(float aspect) {
    _aspect = aspect;
    return *this;
}

SceneBuilder &SceneBuilder::depth(float zNear, float zFar) {
    _zNear = zNear;
    _zFar = zFar;
    return *this;
}

SceneBuilder &SceneBuilder::modelSupplier(const function<shared_ptr<ModelSceneNode>(SceneGraph &)> &supplier) {
    _modelSupplier = supplier;
    return *this;
}

SceneBuilder &SceneBuilder::modelScale(float scale) {
    _modelScale = scale;
    return *this;
}

SceneBuilder &SceneBuilder::modelOffset(glm::vec2 offset) {
    _modelOffset = move(offset);
    return *this;
}

SceneBuilder &SceneBuilder::cameraTransform(glm::mat4 transform) {
    _cameraTransform = move(transform);
    return *this;
}

SceneBuilder &SceneBuilder::cameraFromModelNode(string nodeName) {
    _cameraNodeName = move(nodeName);
    return *this;
}

SceneBuilder &SceneBuilder::ambientLightColor(glm::vec3 color) {
    _ambientLightColor = move(color);
    return *this;
}

SceneBuilder &SceneBuilder::lightingRefFromModelNode(string nodeName) {
    _lightingRefNodeName = move(nodeName);
    return *this;
}

} // namespace gui

} // namespace reone
