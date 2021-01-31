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

#include "../resource/format/binfile.h"

namespace reone {

namespace render {

class LipFile : public resource::BinaryFile {
public:
    struct Keyframe {
        float time { 0.0f };
        uint8_t shape { 0 };
    };

    LipFile();

    const std::vector<Keyframe> &keyframes() const { return _keyframes; }

private:
    std::vector<Keyframe> _keyframes;

    void doLoad() override;
};

} // namespace render

} // namespace reone
