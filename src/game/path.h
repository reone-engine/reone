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

#include <vector>

#include <glm/vec2.hpp>

#include "../resource/gfffile.h"

namespace reone {

namespace game {

class Path {
public:
    struct Point {
        float x { 0.0f };
        float y { 0.0f };
        std::vector<int> adjPoints;
    };

    Path() = default;

    void load(const resource::GffStruct &pth);

    const std::vector<Point> &points() const;

private:
    std::vector<Point> _points;

    Path(const Path &) = delete;
    Path &operator=(const Path &) = delete;
};

} // namespace game

} // namespace reone
