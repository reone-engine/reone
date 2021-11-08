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

#include "../../../resource/format/binreader.h"

namespace reone {

namespace graphics {

class Walkmesh;

class BwmReader : public resource::BinaryReader {
public:
    BwmReader();

    std::shared_ptr<Walkmesh> walkmesh() const { return _walkmesh; }

private:
    enum class WalkmeshType {
        PWK_DWK = 0,
        WOK = 1
    };

    WalkmeshType _type {WalkmeshType::WOK};

    uint32_t _numVertices {0};
    uint32_t _offVertices {0};
    uint32_t _numFaces {0};
    uint32_t _offIndices {0};
    uint32_t _offMaterials {0};
    uint32_t _offNormals {0};
    uint32_t _offPlanarDistances {0};
    uint32_t _numAabb {0};
    uint32_t _offAabb {0};
    uint32_t _numAdjacencies {0};
    uint32_t _offAdjacencies {0};
    uint32_t _numEdges {0};
    uint32_t _offsetEdges {0};
    uint32_t _numPerimeters {0};
    uint32_t _offPerimeters {0};

    std::vector<float> _vertices;
    std::vector<uint32_t> _indices;
    std::vector<uint32_t> _materials;
    std::vector<float> _normals;

    std::shared_ptr<Walkmesh> _walkmesh;

    void doLoad() override;

    void loadVertices();
    void loadIndices();
    void loadMaterials();
    void loadNormals();
    void loadAABB();
};

} // namespace graphics

} // namespace reone
