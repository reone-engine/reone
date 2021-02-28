
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

#include "meshes.h"

#include "glm/ext.hpp"

using namespace std;

namespace reone {

namespace render {

// Quad

static const vector<float> g_quadVertices {
    0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 0.0f
};

static const vector<float> g_quadFlipXVertices {
    0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 1.0f, 0.0f
};

static const vector<float> g_quadFlipYVertices {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 1.0f
};

static const vector<float> g_quadFlipXYVertices {
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f, 1.0f
};

static const vector<float> g_quadNDCVertices {
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f
};

static const vector<uint16_t> g_quadIndices = { 0, 1, 2, 2, 3, 0 };

static const Mesh::VertexOffsets g_quadOffsets = { 0, -1, 3 * sizeof(float), -1, -1, -1, -1, -1, 5 * sizeof(float) };

// END Quad

// Cube

static const vector<float> g_cubeVertices = {
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f
};

static const vector<uint16_t> g_cubeIndices = {
    0, 1, 2, 2, 3, 0,
    2, 4, 5, 5, 3, 2,
    1, 7, 4, 4, 2, 1,
    0, 6, 7, 7, 1, 0,
    7, 6, 5, 5, 4, 7,
    6, 0, 3, 3, 5, 6
};

static const Mesh::VertexOffsets g_cubeOffsets = { 0, -1, -1, -1, -1, -1, -1, -1, 3 * sizeof(float) };

// END Cube

// Billboard

static const vector<float> g_billboardVertices = {
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
     0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
     0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f, 0.0f, 1.0f
};

static const vector<uint16_t> g_billboardIndices = {
    0, 1, 2, 2, 3, 0
};

static Mesh::VertexOffsets g_billboardOffsets = { 0, -1, 3 * sizeof(float), -1, -1, -1, -1, -1, 5 * sizeof(float) };

// END Billboard

// Cubemap

static const vector<float> g_cubemapVertices {
    // back face
    -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
     1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
     1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
    -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
    // front face
    -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
     1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
     1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
    -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
    // left face
    -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
    -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
    -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
    -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
    // right face
     1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
     1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
     1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
     1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
    // bottom face
    -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
     1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
     1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
    -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
    // top face
    -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
     1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
     1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
    -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
};

static const vector<uint16_t> g_cubemapIndices {
    0, 1, 2, 1, 0, 3,
    4, 5, 6, 6, 7, 4,
    8, 9, 10, 10, 11, 8,
    12, 13, 14, 13, 12, 15,
    16, 17, 18, 18, 19, 16,
    20, 21, 22, 21, 20, 23
};

static const Mesh::VertexOffsets g_cubemapOffsets { 0, 3 * sizeof(float), 6 * sizeof(float), -1, -1, -1, -1, -1, 8 * sizeof(float) };

// END Cubemap

// AABB

static const vector<float> g_aabbVertices = {
    -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f, -0.5f,
     0.5f,  0.5f,  0.5f,
     0.5f,  0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f
};

static const vector<uint16_t> g_aabbIndices = {
    0, 1, 1, 2, 2, 3, 3, 0,
    2, 4, 4, 5, 5, 3, 3, 2,
    1, 7, 7, 4, 4, 2, 2, 1,
    0, 6, 6, 7, 7, 1, 1, 0,
    7, 6, 6, 5, 5, 4, 4, 7,
    6, 0, 0, 3, 3, 5, 5, 6
};

static const Mesh::VertexOffsets g_aabbOffsets = { 0, -1, -1, -1, -1, -1, -1, -1, 3 * sizeof(float) };

// END AABB

Meshes &Meshes::instance() {
    static Meshes instance;
    return instance;
}

static unique_ptr<Mesh> getMesh(
    int vertexCount,
    const vector<float> &vertices,
    const vector<uint16_t> &indices,
    const Mesh::VertexOffsets &offsets,
    Mesh::DrawMode mode = Mesh::DrawMode::Triangles) {

    auto mesh = make_unique<Mesh>(vertexCount, vertices, indices, offsets, mode);
    mesh->init();
    return move(mesh);
}

static unique_ptr<Mesh> getSphereMesh() {
    static constexpr int kNumSegmentsX = 16;
    static constexpr int kNumSegmentsY = 16;

    vector<float> vertices;
    vector<uint16_t> indices;
    Mesh::VertexOffsets offsets { 0, 3 * sizeof(float), 5 * sizeof(float), -1, -1, -1, -1, -1, 8 * sizeof(float) };

    for (int y = 0; y <= kNumSegmentsY; ++y) {
        for (int x = 0; x <= kNumSegmentsX; ++x) {
            float xSegment = static_cast<float>(x) / static_cast<float>(kNumSegmentsX);
            float ySegment = static_cast<float>(y) / static_cast<float>(kNumSegmentsY);
            float xPos = glm::cos(xSegment * 2.0f * glm::pi<float>()) * glm::sin(ySegment * glm::pi<float>());
            float yPos = glm::cos(ySegment * glm::pi<float>());
            float zPos = glm::sin(xSegment * 2.0f * glm::pi<float>()) * glm::sin(ySegment * glm::pi<float>());

            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);

            vertices.push_back(xSegment);
            vertices.push_back(ySegment);

            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);
        }
    }

    bool oddRow = false;
    for (int y = 0; y < kNumSegmentsY; ++y) {
        if (!oddRow) { // even rows: y == 0, y == 2; and so on
            for (int x = 0; x <= kNumSegmentsX; ++x) {
                indices.push_back(y * (kNumSegmentsX + 1) + x);
                indices.push_back((y + 1) * (kNumSegmentsX + 1) + x);
            }
        } else {
            for (int x = kNumSegmentsX; x >= 0; --x)
            {
                indices.push_back((y + 1) * (kNumSegmentsX + 1) + x);
                indices.push_back(y * (kNumSegmentsX + 1) + x);
            }
        }
        oddRow = !oddRow;
    }

    auto mesh = make_unique<Mesh>(static_cast<int>(vertices.size()) / 8, move(vertices), move(indices), move(offsets), Mesh::DrawMode::TriangleStrip);
    mesh->init();

    return move(mesh);
}

void Meshes::init() {
    if (!_inited) {
        _quad = getMesh(4, g_quadVertices, g_quadIndices, g_quadOffsets);
        _quadFlipX = getMesh(4, g_quadFlipXVertices, g_quadIndices, g_quadOffsets);
        _quadFlipY = getMesh(4, g_quadFlipYVertices, g_quadIndices, g_quadOffsets);
        _quadFlipXY = getMesh(4, g_quadFlipXYVertices, g_quadIndices, g_quadOffsets);
        _quadNDC = getMesh(4, g_quadNDCVertices, g_quadIndices, g_quadOffsets);
        _cube = getMesh(8, g_cubeVertices, g_cubeIndices, g_cubeOffsets);
        _sphere = getSphereMesh();
        _billboard = getMesh(4, g_billboardVertices, g_billboardIndices, g_billboardOffsets);
        _cubemap = getMesh(24, g_cubemapVertices, g_cubemapIndices, g_cubemapOffsets);
        _aabb = getMesh(8, g_aabbVertices, g_aabbIndices, g_aabbOffsets, Mesh::DrawMode::Lines);

        _inited = true;
    }
}

Meshes::~Meshes() {
    deinit();
}

void Meshes::deinit() {
    if (_inited) {
        _quad.reset();
        _quadFlipX.reset();
        _quadFlipY.reset();
        _quadFlipXY.reset();
        _quadNDC.reset();
        _cube.reset();
        _billboard.reset();
        _cubemap.reset();
        _aabb.reset();

        _inited = false;
    }
}

} // namespace render

} // namespace reone
