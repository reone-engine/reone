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

#include <memory>
#include <string>

#include "../common/types.h"

#include "types.h"

namespace reone {

namespace resource {

/**
 * Common interface to query for game resources.
 */
class IResourceProvider {
public:
    virtual ~IResourceProvider() {
    }

    virtual std::shared_ptr<ByteArray> find(const std::string &resRef, ResourceType type) = 0;

    /**
     * @return true if this resource provider supports the specified ResType,
     *         false otherwise
     */
    virtual bool supports(ResourceType type) const = 0;
};

} // namespace resource

} // namespace reone
