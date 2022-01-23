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

#include "trigger.h"

#include "../../graphics/context.h"
#include "../../graphics/mesh.h"
#include "../../graphics/shaders.h"

using namespace std;

using namespace reone::graphics;

namespace reone {

namespace scene {

void TriggerSceneNode::init() {
    size_t numVertices = _geometry.size();

    vector<glm::vec3> normals(numVertices + 1, glm::vec3(0.0f));
    vector<Mesh::Face> faces;

    glm::vec3 centroid(0.0f);
    for (auto &vert : _geometry) {
        centroid += vert;
    }
    centroid /= static_cast<float>(numVertices);

    for (size_t i = 0; i < numVertices; ++i) {
        int i1 = i + 1;
        int i2 = (i == numVertices - 1) ? 1 : (i + 2);

        auto normal = glm::cross(_geometry[i1 - 1] - centroid, _geometry[i2 - 1] - centroid);
        normals[0] += normal;
        normals[i1] += normal;
        normals[i2] += normal;

        Mesh::Face face;
        face.indices[0] = 0;
        face.indices[1] = i1;
        face.indices[2] = i2;
        face.normal = move(normal);
        face.material = kMaxWalkmeshMaterials - 1;
        faces.push_back(move(face));
    }

    for (size_t i = 0; i < normals.size(); ++i) {
        normals[i] = glm::normalize(normals[i]);
    }

    vector<float> vertices;
    vertices.push_back(centroid.x);
    vertices.push_back(centroid.y);
    vertices.push_back(centroid.z);
    vertices.push_back(normals[0].x);
    vertices.push_back(normals[0].y);
    vertices.push_back(normals[0].z);
    vertices.push_back(1.0f); // material
    for (size_t i = 0; i < numVertices; ++i) {
        vertices.push_back(_geometry[i].x);
        vertices.push_back(_geometry[i].y);
        vertices.push_back(_geometry[i].z);
        vertices.push_back(normals[i + 1].x);
        vertices.push_back(normals[i + 1].y);
        vertices.push_back(normals[i + 1].z);
        vertices.push_back(1.0f); // material
    }

    Mesh::VertexSpec spec;
    spec.stride = 7 * sizeof(float);
    spec.offCoords = 0;
    spec.offNormals = 3 * sizeof(float);
    spec.offMaterial = 6 * sizeof(float);

    _mesh = make_unique<Mesh>(move(vertices), move(faces), move(spec));
    _mesh->init();
}

void TriggerSceneNode::draw() {
    auto &uniforms = _shaders.uniforms();
    uniforms.general.resetLocals();
    uniforms.general.model = _absTransform;

    _shaders.use(_shaders.walkmesh(), true);
    _graphicsContext.withFaceCulling(CullFaceMode::Back, [this]() {
        _mesh->draw();
    });
}

} // namespace scene

} // namespace reone
