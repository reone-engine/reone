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

#include <boost/noncopyable.hpp>

#include "../types.h"

namespace reone {

namespace resource {

class VisFile : boost::noncopyable {
public:
    void load(const std::shared_ptr<std::istream> &in);

    const Visibility &visibility() const { return _visibility; }

private:
    Visibility _visibility;
    std::string _roomFrom;

    void processLine(const std::string &line);
};

} // namespace resource

} // namespace reone
