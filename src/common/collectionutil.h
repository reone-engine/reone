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

#include <functional>
#include <map>
#include <vector>

namespace reone {

template <class Src, class Dest>
std::vector<Dest> transform(const std::vector<Src> &source, const std::function<Dest(const Src &)> &fn) {
    std::vector<Dest> result;
    for (auto &item : source) {
        result.push_back(fn(item));
    }
    return std::move(result);
}

template <class Src, class Dest>
std::map<Src, Dest> associate(const std::vector<Src> &source, const std::function<Dest(const Src &)> &fn) {
    std::map<Src, Dest> result;
    for (auto &item : source) {
        result.insert(make_pair(item, fn(item)));
    }
    return move(result);
}

} // namespace reone
