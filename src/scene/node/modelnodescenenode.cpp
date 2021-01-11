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

#include "modelnodescenenode.h"

#include <stdexcept>

#include "../scenegraph.h"

#include "lightnode.h"
#include "modelscenenode.h"

using namespace std;

using namespace reone::render;

namespace reone {

namespace scene {

ModelNodeSceneNode::ModelNodeSceneNode(SceneGraph *sceneGraph, const ModelSceneNode *modelSceneNode, ModelNode *modelNode) :
    SceneNode(sceneGraph),
    _modelSceneNode(modelSceneNode),
    _modelNode(modelNode) {

    if (!modelSceneNode) {
        throw invalid_argument("modelSceneNode must not be null");
    }
    if (!modelNode) {
        throw invalid_argument("modelNode must not be null");
    }
}

bool ModelNodeSceneNode::shouldRender() const {
    shared_ptr<ModelMesh> mesh(_modelNode->mesh());
    return mesh && mesh->shouldRender() && (mesh->hasDiffuseTexture() || _modelSceneNode->hasTextureOverride());
}

bool ModelNodeSceneNode::shouldCastShadows() const {
    shared_ptr<ModelMesh> mesh(_modelNode->mesh());
    return mesh && !mesh->shouldRender() && mesh->shouldCastShadows();
}

bool ModelNodeSceneNode::isTransparent() const {
    shared_ptr<ModelMesh> mesh(_modelNode->mesh());
    if (!mesh || _modelSceneNode->model()->classification() == Model::Classification::Character) {
        return false;
    }
    return mesh->isTransparent() || _modelNode->alpha() < 1.0f;
}

void ModelNodeSceneNode::renderSingle(bool shadowPass) const {
    shared_ptr<ModelMesh> mesh(_modelNode->mesh());
    if (!mesh) return;

    LocalUniforms locals;
    locals.general.model = _absoluteTransform;
    locals.general.alpha = _modelSceneNode->alpha() * _modelNode->alpha();

    if (!shadowPass) {
        if (mesh->hasEnvmapTexture()) {
            locals.general.envmapEnabled = true;
        }
        if (mesh->hasLightmapTexture()) {
            locals.general.lightmapEnabled = true;
        }
        if (mesh->hasBumpyShinyTexture()) {
            locals.general.bumpyShinyEnabled = true;
        }
        if (mesh->hasBumpmapTexture()) {
            locals.general.bumpmapEnabled = true;
        }
        bool receivesShadows = _modelSceneNode->model()->classification() == Model::Classification::Other;
        if (receivesShadows) {
            locals.general.shadowsEnabled = true;
        }
        shared_ptr<ModelNode::Skin> skin(_modelNode->skin());
        if (skin) {
            locals.general.skeletalEnabled = true;
            locals.skeletal = Shaders::instance().skeletalUniforms();
            locals.skeletal->absTransform = _modelNode->absoluteTransform();
            locals.skeletal->absTransformInv = _modelNode->absoluteTransformInverse();

            for (int i = 0; i < kMaxBoneCount; ++i) {
                locals.skeletal->bones[i] = glm::mat4(1.0f);
            }
            for (auto &pair : skin->nodeIdxByBoneIdx) {
                uint16_t boneIdx = pair.first;
                uint16_t nodeIdx = pair.second;

                ModelNodeSceneNode *bone = _modelSceneNode->getModelNodeByIndex(nodeIdx);
                if (bone) {
                    locals.skeletal->bones[boneIdx] = bone->boneTransform();
                }
            }
        }
        if (_modelNode->isSelfIllumEnabled()) {
            locals.general.selfIllumEnabled = true;
            locals.general.selfIllumColor = glm::vec4(_modelNode->selfIllumColor(), 1.0f);
        }
        if (_modelSceneNode->isLightingEnabled()) {
            const vector<LightSceneNode *> &lights = _modelSceneNode->lightsAffectedBy();

            locals.general.lightingEnabled = true;
            locals.lighting = Shaders::instance().lightingUniforms();
            locals.lighting->ambientLightColor = glm::vec4(_sceneGraph->ambientLightColor(), 1.0f);
            locals.lighting->lightCount = static_cast<int>(lights.size());

            for (int i = 0; i < locals.lighting->lightCount; ++i) {
                ShaderLight &shaderLight = locals.lighting->lights[i];
                shaderLight.position = lights[i]->absoluteTransform()[3];
                shaderLight.radius = lights[i]->radius();
                shaderLight.color = glm::vec4(lights[i]->color(), 1.0f);
            }
        }
    }

    ShaderProgram program = shadowPass ? ShaderProgram::DepthDepth : ShaderProgram::ModelModel;
    Shaders::instance().activate(program, locals);

    mesh->render(_modelSceneNode->textureOverride());
}

const ModelSceneNode *ModelNodeSceneNode::modelSceneNode() const {
    return _modelSceneNode;
}

ModelNode *ModelNodeSceneNode::modelNode() const {
    return _modelNode;
}

const glm::mat4 &ModelNodeSceneNode::boneTransform() const {
    return _boneTransform;
}

void ModelNodeSceneNode::setBoneTransform(const glm::mat4 &transform) {
    _boneTransform = transform;
}

} // namespace scene

} // namespace reone
