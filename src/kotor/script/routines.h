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

#include "../../script/routine.h"
#include "../../script/routines.h"

namespace reone {

namespace game {

struct Services;

class Game;

} // namespace game

namespace kotor {

struct RoutineContext;

class Routines : public script::IRoutines {
public:
    Routines() = default;

    Routines(game::Game &game, game::Services &services) :
        _game(&game),
        _services(&services) {
    }

    void initForKotOR();
    void initForTSL();

    const script::Routine &get(int index) const override;

    int getIndexByName(const std::string &name) const;

private:
    game::Game *_game {nullptr};
    game::Services *_services {nullptr};

    std::vector<script::Routine> _routines;

    void add(
        std::string name,
        script::VariableType retType,
        std::vector<script::VariableType> argTypes,
        script::Variable (*fn)(const std::vector<script::Variable> &args, const RoutineContext &ctx));
};

} // namespace kotor

} // namespace reone