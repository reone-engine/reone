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

#include "../common/cache.h"
#include "../resource/resources.h"
#include "../resource/types.h"

#include "program.h"

namespace reone {

namespace script {

class Scripts : public MemoryCache<std::string, ScriptProgram> {
public:
    Scripts(resource::Resources *resources);

private:
    resource::Resources *_resources;

    std::shared_ptr<ScriptProgram> doGet(std::string resRef);
};

} // namespace script

} // namespace reone
