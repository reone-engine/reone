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

#include "../types.h"

#include "scenenode.h"

namespace reone {

namespace scene {

class LightSceneNode : public SceneNode {
public:
    LightSceneNode(glm::vec3 color, int priority, SceneGraph *sceneGraph);

    bool isShadow() const { return _shadow; }
    bool isAmbientOnly() const { return _ambientOnly; }

    const glm::vec3 &color() const { return _color; }
    int priority() const { return _priority; }
    float multiplier() const { return _multiplier; }
    float radius() const { return _radius; }

    void setMultiplier(float multiplier);
    void setRadius(float radius);
    void setShadow(bool shadow);
    void setAmbientOnly(bool ambientOnly);

private:
    glm::vec3 _color;
    int _priority;

    float _multiplier { 1.0f };
    float _radius { 1.0f };
    bool _shadow { false };
    bool _ambientOnly { false };
};

} // namespace scene

} // namespace reone
