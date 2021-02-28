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

#include "player.h"

#include <stdexcept>

#include "camera/camera.h"
#include "object/area.h"
#include "object/creature.h"
#include "object/module.h"
#include "party.h"

using namespace std;

namespace reone {

namespace game {

Player::Player(Module *module, Area *area, Camera *camera, const Party *party) :
    _module(module), _area(area), _camera(camera), _party(party) {

    if (!module) {
        throw invalid_argument("module must not be null");
    }
    if (!area) {
        throw invalid_argument("area must not be null");
    }
    if (!camera) {
        throw invalid_argument("camera must not be null");
    }
    if (!party) {
        throw invalid_argument("party must not be null");
    }
}

bool Player::handle(const SDL_Event &event) {
    shared_ptr<Creature> partyLeader(_party->getLeader());
    if (!partyLeader) return false;

    switch (event.type) {
        case SDL_KEYDOWN:
            return handleKeyDown(event.key);
        case SDL_KEYUP:
            return handleKeyUp(event.key);
        case SDL_MOUSEBUTTONDOWN:
            return handleMouseButtonDown(event.button);
        case SDL_MOUSEBUTTONUP:
            return handleMouseButtonUp(event.button);
        default:
            return false;
    }
}

bool Player::handleKeyDown(const SDL_KeyboardEvent &event) {
    switch (event.keysym.scancode) {
        case SDL_SCANCODE_W:
            _moveForward = true;
            return true;

        case SDL_SCANCODE_Z:
            _moveLeft = true;
            return true;

        case SDL_SCANCODE_S:
            _moveBackward = true;
            return true;

        case SDL_SCANCODE_C:
            _moveRight = true;
            return true;

        case SDL_SCANCODE_X: {
            shared_ptr<Creature> partyLeader(_party->getLeader());
            partyLeader->playAnimation(CombatAnimation::Draw, partyLeader->getWieldType());
            return true;
        }
        default:
            return false;
    }
}

bool Player::handleKeyUp(const SDL_KeyboardEvent &event) {
    switch (event.keysym.scancode) {
        case SDL_SCANCODE_W:
            _moveForward = false;
            return true;

        case SDL_SCANCODE_Z:
            _moveLeft = false;
            return true;

        case SDL_SCANCODE_S:
            _moveBackward = false;
            return true;

        case SDL_SCANCODE_C:
            _moveRight = false;
            return true;

        default:
            return false;
    }
}

bool Player::handleMouseButtonDown(const SDL_MouseButtonEvent &event) {
    if (_camera->isMouseLookMode() && event.button == SDL_BUTTON_LEFT) {
        _moveForward = true;
        _leftPressedInMouseLook = true;
        return true;
    }

    return false;
}

bool Player::handleMouseButtonUp(const SDL_MouseButtonEvent &event) {
    if (_leftPressedInMouseLook && event.button == SDL_BUTTON_LEFT) {
        _moveForward = false;
        _leftPressedInMouseLook = false;
        return true;
    }

    return false;
}

void Player::update(float dt) {
    shared_ptr<Creature> partyLeader(_party->getLeader());
    if (!partyLeader || partyLeader->isMovementRestricted()) return;

    float facing = 0.0f;
    bool movement = true;

    if (_moveForward) {
        facing = _camera->facing();
    } else if (_moveBackward) {
        facing = _camera->facing() + glm::pi<float>();
    } else if (_moveLeft) {
        facing = _camera->facing() + glm::half_pi<float>();
    } else if (_moveRight) {
        facing = _camera->facing() - glm::half_pi<float>();
    } else {
        movement = false;
    }

    ActionQueue &actions = partyLeader->actionQueue();

    if (movement) {
        actions.clear();

        glm::vec2 dir(glm::normalize(glm::vec2(-glm::sin(facing), glm::cos(facing))));

        if (_area->moveCreature(partyLeader, dir, true, dt)) {
            partyLeader->setMovementType(Creature::MovementType::Run);
        }
    } else if (actions.isEmpty()) {
        partyLeader->setMovementType(Creature::MovementType::None);
    }
}

void Player::stopMovement() {
    _moveForward = false;
    _moveLeft = false;
    _moveBackward = false;
    _moveRight = false;

    shared_ptr<Creature> partyLeader(_party->getLeader());
    partyLeader->setMovementType(Creature::MovementType::None);
}

bool Player::isMovementRequested() const {
    return _moveForward || _moveLeft || _moveBackward || _moveRight;
}

} // namespace game

} // namespace reone
