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

#include "walkmesh.h"

using namespace std;

namespace reone {

namespace graphics {

const Walkmesh::Face *Walkmesh::raycast(
    set<uint32_t> surfaces,
    const glm::vec3 &origin,
    const glm::vec3 &dir,
    float maxDistance,
    float &outDistance) const {

    // For area walkmeshes, find intersection via AABB tree
    if (_rootAabb) {
        return raycastAABB(surfaces, origin, dir, maxDistance, outDistance);
    }

    // For placeable and door walkmeshes, test all faces for intersection
    glm::vec2 baryPosition(0.0f);
    float distance = 0.0f;
    for (auto &face : _faces) {
        if (raycastFace(surfaces, face, origin, dir, maxDistance, distance)) {
            outDistance = distance;
            return &face;
        }
    }

    return nullptr;
}

const Walkmesh::Face *Walkmesh::raycastAABB(
    set<uint32_t> surfaces,
    const glm::vec3 &origin,
    const glm::vec3 &dir,
    float maxDistance,
    float &outDistance) const {

    float distance = 0.0f;

    stack<AABB *> aabbs;
    aabbs.push(_rootAabb.get());

    auto invDir = 1.0f / dir;

    while (!aabbs.empty()) {
        auto aabb = aabbs.top();
        aabbs.pop();

        // Test ray/face intersection for tree leafs
        if (aabb->faceIdx != -1) {
            const Face &face = _faces[aabb->faceIdx];
            if (raycastFace(surfaces, face, origin, dir, maxDistance, distance)) {
                outDistance = distance;
                return &face;
            }
            continue;
        }

        // Test ray/AABB intersection
        if (!aabb->value.raycast(origin, invDir, maxDistance, distance)) {
            continue;
        }

        // Find intersection with child AABB nodes
        if (aabb->child1) {
            aabbs.push(aabb->child1.get());
        }
        if (aabb->child2) {
            aabbs.push(aabb->child2.get());
        }
    }

    return nullptr;
}

bool Walkmesh::raycastFace(
    set<uint32_t> surfaces,
    const Face &face,
    const glm::vec3 &origin,
    const glm::vec3 &dir,
    float maxDistance,
    float &outDistance) const {

    if (surfaces.count(face.material) == 0) {
        return false;
    }

    const glm::vec3 &p0 = face.vertices[0];
    const glm::vec3 &p1 = face.vertices[1];
    const glm::vec3 &p2 = face.vertices[2];

    glm::vec2 baryPosition(0.0f);
    float distance = 0.0f;

    if (glm::intersectRayTriangle(origin, dir, p0, p1, p2, baryPosition, distance) && distance > 0.0f && distance < maxDistance) {
        outDistance = distance;
        return true;
    }

    return false;
}

} // namespace graphics

} // namespace reone
