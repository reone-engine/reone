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

#include "../../resource/types.h"

#include "../types.h"

namespace reone {

namespace resource {

class Resources;

}

namespace graphics {

class Walkmesh;

class Walkmeshes : boost::noncopyable {
public:
    Walkmeshes(resource::Resources &resources);

    void invalidateCache();

    std::shared_ptr<Walkmesh> get(const std::string &resRef, resource::ResourceType type);

private:
    resource::Resources &_resources;

    std::unordered_map<std::string, std::shared_ptr<Walkmesh>> _cache;

    std::shared_ptr<Walkmesh> doGet(const std::string &resRef, resource::ResourceType type);
};

} // namespace graphics

} // namespace reone
