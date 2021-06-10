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

#include "pch.h"
#include "resources.h"
#include "strings.h"

namespace reone {

namespace resource {

class ResourceServices : boost::noncopyable {
public:
    ResourceServices(boost::filesystem::path gamePath);

    void init();

    Resources &resources() { return *_resources; }
    Strings &strings() { return *_strings; }

private:
    boost::filesystem::path _gamePath;

    std::unique_ptr<Resources> _resources;
    std::unique_ptr<Strings> _strings;
};

} // namespace resource

} // namespace reone
