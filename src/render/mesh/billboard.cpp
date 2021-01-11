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

#include "billboard.h"

using namespace std;

namespace reone {

namespace render {

static vector<float> g_vertices = {
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
     0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
     0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f, 0.0f, 1.0f
};

static vector<uint16_t> g_indices = {
    0, 1, 2, 2, 3, 0
};

static Mesh::VertexOffsets g_offsets = { 0, -1, 3 * sizeof(float), -1, -1, -1, 5 * sizeof(float) };

BillboardMesh &BillboardMesh::instance() {
    static BillboardMesh mesh;
    return mesh;
}

BillboardMesh::BillboardMesh() {
    _vertices = move(g_vertices);
    _indices = move(g_indices);
    _offsets = move(g_offsets);
}

} // namespace render

} // namespace reone
