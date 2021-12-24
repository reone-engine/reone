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

#include "../../graphics/camera.h"

#include "../node.h"

namespace reone {

namespace scene {

class CameraSceneNode : public SceneNode {
public:
    CameraSceneNode(SceneGraph &sceneGraph) :
        SceneNode(SceneNodeType::Camera, sceneGraph) {
    }

    bool isInFrustum(const SceneNode &other) const;

    std::shared_ptr<graphics::Camera> camera() const { return _camera; }

    void setOrthographicProjection(float left, float right, float bottom, float top, float zNear, float zFar);
    void setPerspectiveProjection(float fovy, float aspect, float zNear, float zFar);

private:
    std::shared_ptr<graphics::Camera> _camera;

    void onAbsoluteTransformChanged() override;
};

} // namespace scene

} // namespace reone
