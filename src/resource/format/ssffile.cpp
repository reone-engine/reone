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

#include "ssffile.h"

#include <stdexcept>

using namespace std;

namespace reone {

namespace resource {

SsfFile::SsfFile() : BinaryFile(8, "SSF V1.1") {
}

void SsfFile::doLoad() {
    uint32_t tableOffset = readUint32();
    int entryCount = static_cast<int>((_size - tableOffset) / 4);
    seek(tableOffset);
    _soundSet = readArray<uint32_t>(entryCount);
}

} // namespace resource

} // namespace reone
