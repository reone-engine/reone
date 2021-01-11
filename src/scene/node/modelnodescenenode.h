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

#include "scenenode.h"

#include "../../render/model/model.h"

namespace reone {

namespace render {

class ModelNode;

}
namespace scene {

class ModelSceneNode;

class ModelNodeSceneNode : public SceneNode {
public:
    ModelNodeSceneNode(SceneGraph *sceneGraph, const ModelSceneNode *modelSceneNode, render::ModelNode *modelNode);

    void renderSingle(bool shadowPass) const override;

    bool shouldRender() const;
    bool shouldCastShadows() const;

    bool isTransparent() const;

    const ModelSceneNode *modelSceneNode() const;
    render::ModelNode *modelNode() const;
    const glm::mat4 &boneTransform() const;

    void setBoneTransform(const glm::mat4 &transform);

private:
    const ModelSceneNode *_modelSceneNode { nullptr };
    render::ModelNode *_modelNode { nullptr };
    glm::mat4 _animTransform { 1.0f };
    glm::mat4 _boneTransform { 1.0f };
};

} // namespace scene

} // namespace reone
