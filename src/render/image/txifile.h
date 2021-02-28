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

#include <istream>
#include <memory>

#include "../texture.h"

namespace reone {

namespace render {

class TxiFile {
public:
    void load(const std::shared_ptr<std::istream> &in);

    const Texture::Features &features() const { return _features; }

private:
    enum class State {
        None,
        UpperLeftCoords,
        LowerRightCoords
    };

    Texture::Features _features;
    State _state { State::None };
    int _upperLeftCoordCount { 0 };
    int _lowerRightCoordCount { 0 };

    void processLine(const std::vector<std::string> &tokens);
    Texture::Blending parseBlending(const std::string &s) const;
};

} // namespace render

} // namespace reone
