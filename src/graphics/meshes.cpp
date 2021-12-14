
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

#include "mesh.h"

using namespace std;

namespace reone {

namespace graphics {

// Quads

static const vector<float> g_quadVertices {
    0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f, 0.0f};

static const vector<float> g_quadNDCVertices {
    -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f};

static const vector<float> g_billboardVertices {
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
    -0.5f, 0.5f, 0.0f, 0.0f, 1.0f};

static const vector<float> g_grassVertices {
    -0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.5f, 1.0f, 0.0f, 1.0f, 1.0f,
    -0.5f, 1.0f, 0.0f, 0.0f, 1.0f};

static const vector<Mesh::Face> g_quadFaces {
    Mesh::Face(0, 1, 2),
    Mesh::Face(2, 3, 0)};

static const Mesh::VertexSpec g_quadSpec {5 * sizeof(float), 0, -1, 3 * sizeof(float)};

// END Quads

// Cubemap

static const vector<float> g_cubemapVertices {
    // back face
    -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
    1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,   // top-right
    1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,  // bottom-right
    -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,  // top-left
    // front face
    -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
    1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  // bottom-right
    1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // top-right
    -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // top-left
    // left face
    -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-right
    -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // top-left
    -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
    -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // bottom-right
    // right face
    1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-left
    1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
    1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // top-right
    1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // bottom-left
    // bottom face
    -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
    1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // top-left
    1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom-left
    -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // bottom-right
    // top face
    -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
    1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // bottom-right
    1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // top-right
    -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f   // bottom-left
};

static const vector<Mesh::Face> g_cubemapFaces {
    Mesh::Face(0, 1, 2),
    Mesh::Face(1, 0, 3),
    Mesh::Face(4, 5, 6),
    Mesh::Face(6, 7, 4),
    Mesh::Face(8, 9, 10),
    Mesh::Face(10, 11, 8),
    Mesh::Face(12, 13, 14),
    Mesh::Face(13, 12, 15),
    Mesh::Face(16, 17, 18),
    Mesh::Face(18, 19, 16),
    Mesh::Face(20, 21, 22),
    Mesh::Face(21, 20, 23)};

static const Mesh::VertexSpec g_cubemapSpec {8 * sizeof(float), 0, 3 * sizeof(float), 6 * sizeof(float)};

// END Cubemap

static unique_ptr<Mesh> getMesh(vector<float> vertices, vector<Mesh::Face> faces, Mesh::VertexSpec spec) {
    auto mesh = make_unique<Mesh>(move(vertices), move(faces), move(spec));
    mesh->init();
    return move(mesh);
}

void Meshes::init() {
    if (_inited) {
        return;
    }
    _quad = getMesh(g_quadVertices, g_quadFaces, g_quadSpec);
    _quadNDC = getMesh(g_quadNDCVertices, g_quadFaces, g_quadSpec);
    _billboard = getMesh(g_billboardVertices, g_quadFaces, g_quadSpec);
    _grass = getMesh(g_grassVertices, g_quadFaces, g_quadSpec);
    _cubemap = getMesh(g_cubemapVertices, g_cubemapFaces, g_cubemapSpec);

    _inited = true;
}

void Meshes::deinit() {
    if (!_inited) {
        return;
    }
    _quad.reset();
    _quadNDC.reset();
    _billboard.reset();
    _grass.reset();
    _cubemap.reset();

    _inited = false;
}

} // namespace graphics

} // namespace reone
