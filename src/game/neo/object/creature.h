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

namespace resource {

class Gff;

}

namespace game {

namespace neo {

class Creature : public Object {
public:
    enum class State {
        Pause,
        Walk,
        Run
    };

    Creature(
        uint32_t id,
        ObjectFactory &objectFactory,
        GameServices &gameSvc,
        graphics::GraphicsOptions &graphicsOpt,
        graphics::GraphicsServices &graphicsSvc,
        resource::ResourceServices &resourceSvc) :
        Object(
            id,
            ObjectType::Creature,
            objectFactory,
            gameSvc,
            graphicsOpt,
            graphicsSvc,
            resourceSvc) {
    }

    void loadFromGit(const resource::Gff &git);
    void loadFromUtc(const std::string &templateResRef);

    bool moveForward(float delta);

    void setState(State state) {
        _state = state;
    }

    // Object

    void update(float delta) override;

    // END Object

private:
    State _state {State::Pause};
};

} // namespace neo

} // namespace game

} // namespace reone