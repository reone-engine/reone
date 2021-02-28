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

#include <stack>
#include <stdexcept>

#include "../../common/log.h"
#include "../../render/featureutil.h"
#include "../../render/meshes.h"
#include "../../resource/resources.h"

#include "../scenegraph.h"
#include "../types.h"

#include "cameranode.h"
#include "emitternode.h"
#include "lightnode.h"
#include "modelnodescenenode.h"
#include "modelscenenode.h"

using namespace std;
using namespace std::placeholders;

using namespace reone::render;
using namespace reone::resource;

namespace reone {

namespace scene {

static constexpr int kSelfIlluminatedPriority = 5;

static bool g_drawAABB = false;

ModelSceneNode::ModelSceneNode(Classification classification, const shared_ptr<Model> &model, SceneGraph *sceneGraph, set<string> ignoreNodes) :
    SceneNode(sceneGraph),
    _classification(classification),
    _model(model),
    _animator(this, ignoreNodes) {

    initModelNodes();

    _volumetric = true;
}

static bool validateEmitter(const Emitter &emitter) {
    switch (emitter.updateMode()) {
        case Emitter::UpdateMode::Fountain:
        case Emitter::UpdateMode::Single:
        case Emitter::UpdateMode::Explosion:
            break;
        default:
            warn("validateEmitter: unsupported update mode: " + to_string(static_cast<int>(emitter.updateMode())));
            return false;
    }

    switch (emitter.renderMode()) {
        case Emitter::RenderMode::Normal:
        case Emitter::RenderMode::BillboardToWorldZ:
        case Emitter::RenderMode::MotionBlur:
        case Emitter::RenderMode::BillboardToLocalZ:
        case Emitter::RenderMode::AlignedToParticleDir:
            break;
        default:
            warn("validateEmitter: unsupported render mode: " + to_string(static_cast<int>(emitter.renderMode())));
            return false;
    }

    switch (emitter.blendMode()) {
        case Emitter::BlendMode::Normal:
        case Emitter::BlendMode::Lighten:
            break;
        default:
            warn("validateEmitter: unsupported blend mode: " + to_string(static_cast<int>(emitter.blendMode())));
            return false;
    }

    return true;
}

void ModelSceneNode::initModelNodes() {
    shared_ptr<ModelNodeSceneNode> rootNode(getModelNodeSceneNode(*_model->rootNode()));
    addChild(rootNode);

    stack<ModelNodeSceneNode *> nodes;
    nodes.push(rootNode.get());

    while (!nodes.empty()) {
        ModelNodeSceneNode *sceneNode = nodes.top();
        nodes.pop();

        const ModelNode *modelNode = sceneNode->modelNode();
        _modelNodeByIndex.insert(make_pair(modelNode->index(), sceneNode));
        _modelNodeByNumber.insert(make_pair(modelNode->nodeNumber(), sceneNode));

        for (auto &child : modelNode->children()) {
            shared_ptr<ModelNodeSceneNode> childNode(getModelNodeSceneNode(*child));
            addChild(childNode);
            nodes.push(childNode.get());

            // Optionally convert self-illuminated model nodes to light sources
            if (isFeatureEnabled(Feature::SelfIllumAsLights)) {
                glm::vec3 color;
                if (child->getSelfIllumColor(0.0f, color) && glm::dot(color, color) > 0.0f) {
                    float radius;
                    shared_ptr<ModelMesh> mesh(child->mesh());
                    if (mesh) {
                        glm::vec3 size(mesh->mesh()->aabb().getSize());
                        radius = glm::max(1.0f, glm::sqrt(glm::dot(size, size)));
                    } else {
                        radius = 1.0f;
                    }
                    auto lightNode = make_shared<LightSceneNode>(move(color), kSelfIlluminatedPriority, _sceneGraph);
                    lightNode->setRadius(radius);
                    childNode->addChild(lightNode);
                }
            }

            shared_ptr<ModelNode::Light> light(child->light());
            if (light) {
                auto lightNode = make_shared<LightSceneNode>(child->color(), light->priority, _sceneGraph);
                lightNode->setMultiplier(child->multiplier());
                lightNode->setRadius(child->radius());
                lightNode->setShadow(light->shadow);
                lightNode->setAmbientOnly(light->ambientOnly);
                childNode->addChild(lightNode);
            }

            shared_ptr<Emitter> emitter(child->emitter());
            if (emitter && validateEmitter(*emitter)) {
                auto emitterNode = make_shared<EmitterSceneNode>(this, emitter, _sceneGraph);
                childNode->addChild(emitterNode);
                _emitters.push_back(emitterNode);
            }
        }
    }

    refreshAABB();
}

unique_ptr<ModelNodeSceneNode> ModelSceneNode::getModelNodeSceneNode(ModelNode &node) const {
    auto sceneNode = make_unique<ModelNodeSceneNode>(_sceneGraph, this, &node);
    sceneNode->setLocalTransform(node.absoluteTransform());
    return move(sceneNode);
}

void ModelSceneNode::update(float dt) {
    if (!_visible || !_onScreen) return;

    _animator.update(dt);

    SceneNode::update(dt);
}

void ModelSceneNode::render() {
    if (g_drawAABB) {
        glm::mat4 transform(_absoluteTransform * _aabb.transform());

        ShaderUniforms uniforms(_sceneGraph->uniformsPrototype());
        uniforms.general.model = move(transform);
        Shaders::instance().activate(ShaderProgram::ModelColor, uniforms);

        Meshes::instance().getAABB()->render();
    }
}

shared_ptr<ModelSceneNode> ModelSceneNode::attach(const string &parent, const shared_ptr<Model> &model, ModelSceneNode::Classification classification) {
    ModelNodeSceneNode *parentNode = getModelNode(parent);
    if (!parentNode) return nullptr;

    const ModelNode *parentModelNode = parentNode->modelNode();
    uint16_t parentNumber = parentModelNode->nodeNumber();

    auto maybeAttached = _attachedModels.find(parentNumber);
    if (maybeAttached != _attachedModels.end()) {
        parentNode->removeChild(*maybeAttached->second);
        _attachedModels.erase(maybeAttached);
    }
    if (model) {
        set<string> ignoreNodes;
        for (const ModelNode *node = parentModelNode; node; node = node->parent()) {
            ignoreNodes.insert(node->name());
        }
        auto modelNode = make_shared<ModelSceneNode>(classification, model, _sceneGraph, ignoreNodes);
        parentNode->addChild(modelNode);

        return _attachedModels.insert(make_pair(parentNumber, move(modelNode))).first->second;
    }

    return nullptr;
}

ModelNodeSceneNode *ModelSceneNode::getModelNode(const string &name) const {
    shared_ptr<ModelNode> modelNode(_model->findNodeByName(name));
    if (!modelNode) return nullptr;

    auto maybeSceneNode = _modelNodeByNumber.find(modelNode->nodeNumber());
    if (maybeSceneNode == _modelNodeByNumber.end()) return nullptr;

    return maybeSceneNode->second;
}

ModelNodeSceneNode *ModelSceneNode::getModelNodeByIndex(int index) const {
    auto maybeNode = _modelNodeByIndex.find(index);
    if (maybeNode == _modelNodeByIndex.end()) return nullptr;

    return maybeNode->second;
}

shared_ptr<ModelSceneNode> ModelSceneNode::getAttachedModel(const string &parent) const {
    shared_ptr<ModelNode> parentModelNode(_model->findNodeByName(parent));
    if (!parentModelNode) {
        warn("ModelSceneNode: parent node not found: " + parent);
        return nullptr;
    }
    auto maybeAttached = _attachedModels.find(parentModelNode->nodeNumber());
    return maybeAttached != _attachedModels.end() ? maybeAttached->second : nullptr;
}

void ModelSceneNode::attach(const string &parent, const shared_ptr<SceneNode> &node) {
    shared_ptr<ModelNode> parentModelNode(_model->findNodeByName(parent));
    if (!parentModelNode) {
        warn("ModelSceneNode: parent node not found: " + parent);
        return;
    }
    uint16_t parentNumber = parentModelNode->nodeNumber();

    auto maybeNode = _modelNodeByNumber.find(parentNumber);
    if (maybeNode != _modelNodeByNumber.end()) {
        maybeNode->second->addChild(node);
    }
}

void ModelSceneNode::updateAbsoluteTransform() {
    SceneNode::updateAbsoluteTransform();
    _lightingDirty = true;
}

void ModelSceneNode::updateLighting() {
    if (!_lightingDirty) return;

    _lightsAffectedBy.clear();
    _sceneGraph->getLightsAt(*this, _lightsAffectedBy, _sceneGraph->options().numLights, bind(&ModelSceneNode::isAffectableByLight, this, _1));
    _lightingDirty = false;

    for (auto &attached : _attachedModels) {
        attached.second->setLightsAffectedBy(_lightsAffectedBy);
    }
}

bool ModelSceneNode::isAffectableByLight(const LightSceneNode &light) const {
    if (light.isAmbientOnly()) {
        return _classification == ModelSceneNode::Classification::Room;
    }
    return true;
}

void ModelSceneNode::setLightingIsDirty() {
    _lightingDirty = true;
}

bool ModelSceneNode::getNodeAbsolutePosition(const string &name, glm::vec3 &position) const {
    shared_ptr<ModelNode> node(_model->findNodeByName(name));
    if (!node) {
        shared_ptr<Model> superModel(_model->superModel());
        if (superModel) {
            node = superModel->findNodeByName(name);
        }
    }
    if (!node) return false;

    position = node->absoluteTransform()[3];

    return true;
}

glm::vec3 ModelSceneNode::getCenterOfAABB() const {
    return _absoluteTransform * glm::vec4(_aabb.center(), 1.0f);
}

const string &ModelSceneNode::getName() const {
    return _model->name();
}

void ModelSceneNode::setDiffuseTexture(const shared_ptr<Texture> &texture) {
    for (auto &child : _children) {
        auto modelNode = dynamic_pointer_cast<ModelNodeSceneNode>(child);
        if (modelNode) {
            modelNode->setDiffuseTexture(texture);
        }
    }
}

void ModelSceneNode::setVisible(bool visible) {
    if (_visible == visible) return;

    _visible = visible;

    for (auto &attached : _attachedModels) {
        attached.second->setVisible(visible);
    }
    if (visible) {
        _lightingDirty = true;
    }
}

void ModelSceneNode::setOnScreen(bool onScreen) {
    _onScreen = onScreen;

    for (auto &attached : _attachedModels) {
        attached.second->setOnScreen(onScreen);
    }
}

void ModelSceneNode::setAlpha(float alpha) {
    _alpha = alpha;

    for (auto &attached : _attachedModels) {
        attached.second->setAlpha(alpha);
    }
}

void ModelSceneNode::setProjectileSpeed(float speed) {
    _projectileSpeed = speed;
}

void ModelSceneNode::setLightsAffectedBy(const vector<LightSceneNode *> &lights) {
    _lightsAffectedBy = lights;
}

void ModelSceneNode::refreshAABB() {
    _aabb.reset();

    stack<SceneNode *> nodes;
    nodes.push(this);

    while (!nodes.empty()) {
        SceneNode *node = nodes.top();
        nodes.pop();

        auto modelNodeSceneNode = dynamic_cast<ModelNodeSceneNode *>(node);
        if (modelNodeSceneNode) {
            shared_ptr<ModelMesh> mesh(modelNodeSceneNode->modelNode()->mesh());
            if (mesh) {
                _aabb.expand(mesh->mesh()->aabb() * node->localTransform());
            }
        }

        for (auto &child : node->children()) {
            nodes.push(child.get());
        }
    }
}

void ModelSceneNode::signalEvent(const string &name) {
    debug(boost::format("ModelSceneNode: event signalled: %s %s") % name % _model->name(), 2);

    if (name == "detonate") {
        for (auto &emitter : _emitters) {
            emitter->detonate();
        }
    }
}

} // namespace scene

} // namespace reone
