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

#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

#include "../render/texture.h"
#include "../resource/gfffile.h"

namespace reone {

namespace game {

class Game;

class Map {
public:
    enum class Mode {
        Default,
        Minimap
    };

    Map(Game *game);

    void load(const std::string &area, const resource::GffStruct &gffs);
    void render(Mode mode, const glm::vec4 &bounds) const;

private:
    int _northAxis { 0 };
    glm::vec2 _worldPoint1 { 0.0f };
    glm::vec2 _worldPoint2 { 0.0f };
    glm::vec2 _mapPoint1 { 0.0f };
    glm::vec2 _mapPoint2 { 0.0f };

    Game *_game { nullptr };
    std::shared_ptr<render::Texture> _texture;
    std::shared_ptr<render::Texture> _arrow;

    void loadProperties(const resource::GffStruct &gffs);
    void loadTexture(const std::string &area);
    void loadArrow();

    void drawArea(Mode mode, const glm::vec4 &bounds) const;
    void drawPartyLeader(Mode mode, const glm::vec4 &bounds) const;

    glm::vec2 getMapPosition(const glm::vec2 &world) const;
};

} // namespace game

} // namespace reone
