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

#include <boost/noncopyable.hpp>

#include "../resource/types.h"

#include "types.h"

namespace reone {

namespace render {

class Texture;

class Textures : boost::noncopyable {
public:
    static Textures &instance();

    void init(resource::GameID gameId);
    void invalidateCache();

    std::shared_ptr<Texture> get(const std::string &resRef, TextureUsage usage = TextureUsage::Default);

    std::shared_ptr<Texture> getDefault() const { return _default; }

private:
    resource::GameID _gameId { resource::GameID::KotOR };
    std::shared_ptr<render::Texture> _default;
    std::unordered_map<std::string, std::shared_ptr<Texture>> _cache;

    std::shared_ptr<Texture> doGet(const std::string &resRef, TextureUsage usage);
};

} // namespace render

} // namespace reone
