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

#include "lipwriter.h"

#include "../../common/streamwriter.h"

namespace fs = boost::filesystem;

using namespace std;

namespace reone {

namespace graphics {

LipWriter::LipWriter(LipAnimation &&animation) :
    _animation(animation) {
}

void LipWriter::save(const fs::path &path) {
    auto lip = make_shared<fs::ofstream>(path, ios::binary);

    StreamWriter writer(lip);
    writer.putString("LIP V1.0");
    writer.putFloat(_animation.length());
    writer.putUint32(static_cast<uint32_t>(_animation.keyframes().size()));
    for (auto &keyframe : _animation.keyframes()) {
        writer.putFloat(keyframe.time);
        writer.putByte(keyframe.shape);
    }
}

} // namespace graphics

} // namespace reone
