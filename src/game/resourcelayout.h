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

#include "types.h"

namespace reone {

namespace resource {

struct ResourceServices;

}

namespace game {

struct Options;

class ResourceLayout : boost::noncopyable {
public:
    ResourceLayout(GameID gameId, Options &options, resource::ResourceServices &resourceSvc) :
        _gameId(gameId),
        _options(options),
        _resourceSvc(resourceSvc) {
    }

    void init();

    void loadModuleResources(const std::string &moduleName);

private:
    GameID _gameId;
    Options &_options;
    resource::ResourceServices &_resourceSvc;

    void initForKotOR();
    void initForTSL();
};

} // namespace game

} // namespace reone
