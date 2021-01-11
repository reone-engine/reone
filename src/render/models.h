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

#include <string>
#include <memory>
#include <unordered_map>

#include "../resource/types.h"

#include "types.h"

namespace reone {

namespace render {

class Model;

class Models {
public:
    static Models &instance();

    void init(resource::GameVersion version);
    void invalidateCache();

    std::shared_ptr<Model> get(const std::string &resRef);

private:
    resource::GameVersion _version { resource::GameVersion::KotOR };
    std::unordered_map<std::string, std::shared_ptr<Model>> _cache;

    Models() = default;
    Models(const Models &) = delete;
    Models &operator=(const Models &) = delete;

    std::shared_ptr<Model> doGet(const std::string &resRef);
};

} // namespace render

} // namespace reone
