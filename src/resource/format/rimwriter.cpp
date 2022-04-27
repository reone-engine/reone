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

#include "rimwriter.h"

#include "../../common/streamwriter.h"

using namespace std;

namespace fs = boost::filesystem;

namespace reone {

namespace resource {

void RimWriter::add(Resource &&res) {
    _resources.push_back(res);
}

void RimWriter::save(const fs::path &path) {
    auto rim = make_shared<fs::ofstream>(path, ios::binary);
    save(rim);
}

void RimWriter::save(shared_ptr<ostream> out) {
    StreamWriter writer(out);
    uint32_t numResources = static_cast<uint32_t>(_resources.size());

    writer.putString("RIM V1.0");
    writer.putUint32(0); // reserved
    writer.putUint32(numResources);
    writer.putUint32(0x78); // offset to resource headers
    writer.putBytes(100);   // reserved

    uint32_t id = 0;
    uint32_t offset = 0x78 + numResources * 32;

    // Write resource headers
    for (auto &res : _resources) {
        auto size = static_cast<uint32_t>(res.data.size());

        string resRef(res.resRef);
        resRef.resize(16);
        writer.putString(resRef);

        writer.putUint32(static_cast<uint32_t>(res.resType));
        writer.putUint32(id++);
        writer.putUint32(offset);
        writer.putUint32(size);

        offset += size;
    }

    // Write resources data
    for (auto &res : _resources) {
        writer.putBytes(res.data);
    }
}

} // namespace resource

} // namespace reone
