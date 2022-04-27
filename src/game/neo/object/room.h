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

#include "../object.h"

namespace reone {

namespace game {

namespace neo {

class Room : public Object {
public:
    class Builder : public Object::Builder<Room, Builder> {
    public:
        std::unique_ptr<Room> build() override {
            return std::make_unique<Room>(_id, _tag, _sceneNode);
        }

    private:
        std::string _modelName;
    };

    Room(
        uint32_t id,
        std::string tag,
        std::shared_ptr<scene::SceneNode> sceneNode) :
        Object(
            id,
            ObjectType::Room,
            std::move(tag),
            std::move(sceneNode)) {
    }
};

} // namespace neo

} // namespace game

} // namespace reone