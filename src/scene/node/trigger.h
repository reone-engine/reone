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

#include "../node.h"

namespace reone {

namespace graphics {

class GraphicsContext;
class Mesh;
class Shaders;
class UniformBuffers;

} // namespace graphics

namespace scene {

class TriggerSceneNode : public SceneNode {
public:
    TriggerSceneNode(
        std::vector<glm::vec3> geometry,
        SceneGraph &sceneGraph,
        graphics::GraphicsContext &graphicsContext,
        graphics::Shaders &shaders,
        graphics::UniformBuffers &uniformBuffers) :
        SceneNode(SceneNodeType::Trigger, sceneGraph),
        _geometry(std::move(geometry)),
        _graphicsContext(graphicsContext),
        _shaders(shaders),
        _uniformBuffers(uniformBuffers) {

        init();
    }

    void init();
    void draw();

    bool isIn(const glm::vec2 &pt) const;

private:
    std::vector<glm::vec3> _geometry;

    graphics::GraphicsContext &_graphicsContext;
    graphics::Shaders &_shaders;
    graphics::UniformBuffers &_uniformBuffers;

    std::unique_ptr<graphics::Mesh> _mesh;
};

} // namespace scene

} // namespace reone
