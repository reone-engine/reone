/*
 * Copyright (c) 2020-2023 The reone project contributors
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

#include "reone/system/memorycache.h"

#include "types.h"

namespace reone {

namespace resource {

class Resources;

}

namespace game {

class IVisibilities {
public:
    virtual ~IVisibilities() = default;
};

class Visibilities : public IVisibilities, public MemoryCache<std::string, Visibility> {
public:
    Visibilities(resource::Resources &resources) :
        MemoryCache(std::bind(&Visibilities::doGet, this, std::placeholders::_1)),
        _resources(resources) {
    }

private:
    resource::Resources &_resources;

    std::shared_ptr<Visibility> doGet(std::string resRef);
};

} // namespace game

} // namespace reone