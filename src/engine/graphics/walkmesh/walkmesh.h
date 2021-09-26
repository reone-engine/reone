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

#include "../aabb.h"
#include "../types.h"

namespace reone {

namespace graphics {

class BwmReader;

class Walkmesh : boost::noncopyable {
public:
    struct Face {
        int index {0};
        uint32_t material {0};
        std::vector<glm::vec3> vertices;
        glm::vec3 normal {0.0f};
    };

    bool raycastWalkableFirst(const glm::vec3 &origin, const glm::vec3 &dir, float &distance, int &material) const;
    bool raycastNonWalkableFirst(const glm::vec3 &origin, const glm::vec3 &dir, float &distance, glm::vec3 &normal) const;
    bool raycastNonWalkableClosest(const glm::vec3 &origin, const glm::vec3 &dir, float &distance, glm::vec3 &normal) const;

    const std::vector<Face> &grassFaces() const { return _grassFaces; }
    const AABB &aabb() const { return _aabb; }

private:
    std::vector<Face> _walkableFaces;
    std::vector<Face> _nonWalkableFaces;
    std::vector<Face> _grassFaces;

    AABB _aabb;

    void computeAABB();

    friend class BwmReader;
};

} // namespace graphics

} // namespace reone
