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

#include <boost/noncopyable.hpp>

#include "SDL2/SDL_events.h"

namespace reone {

namespace game {

class Area;
class Camera;
class Creature;
class Module;
class Party;

/**
 * Encapsulates third-person player controls.
 */
class Player : boost::noncopyable {
public:
    Player(Module *module, Area *area, Camera *camera, const Party *party);

    bool handle(const SDL_Event &event);
    void update(float dt);

    void stopMovement();

    bool isMovementRequested() const;

private:
    Module *_module;
    Area *_area;
    Camera *_camera;
    const Party *_party;

    bool _moveForward { false };
    bool _moveLeft { false };
    bool _moveBackward { false };
    bool _moveRight { false };
    bool _leftPressedInMouseLook { false };

    bool handleKeyDown(const SDL_KeyboardEvent &event);
    bool handleKeyUp(const SDL_KeyboardEvent &event);
    bool handleMouseButtonDown(const SDL_MouseButtonEvent &event);
    bool handleMouseButtonUp(const SDL_MouseButtonEvent &event);
};

} // namespace game

} // namespace reone
