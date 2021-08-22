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

/** @file
 *  Implementation of stealth-related routines.
 */

#include "declarations.h"

#include "../../../../script/exception/notimplemented.h"
#include "../../../../script/types.h"

#include "../../../game.h"

#include "argutil.h"

using namespace std;

using namespace reone::script;

namespace reone {

namespace game {

namespace routine {

Variable getMaxStealthXP(Game &game, const vector<Variable> &args, ExecutionContext &ctx) {
    return Variable::ofInt(game.module()->area()->maxStealthXP());
}

Variable setMaxStealthXP(Game &game, const vector<Variable> &args, ExecutionContext &ctx) {
    int max = getInt(args, 0);
    game.module()->area()->setMaxStealthXP(max);
    return Variable();
}

Variable getCurrentStealthXP(Game &game, const vector<Variable> &args, ExecutionContext &ctx) {
    return Variable::ofInt(game.module()->area()->currentStealthXP());
}

Variable setCurrentStealthXP(Game &game, const vector<Variable> &args, ExecutionContext &ctx) {
    int current = getInt(args, 0);
    game.module()->area()->setCurrentStealthXP(current);
    return Variable();
}

Variable awardStealthXP(Game &game, const vector<Variable> &args, ExecutionContext &ctx) {
    throw NotImplementedException();
}

Variable getStealthXPEnabled(Game &game, const vector<Variable> &args, ExecutionContext &ctx) {
    bool result = game.module()->area()->isStealthXPEnabled();
    return Variable::ofInt(static_cast<int>(result));
}

Variable setStealthXPEnabled(Game &game, const vector<Variable> &args, ExecutionContext &ctx) {
    bool enabled = getBool(args, 0);
    game.module()->area()->setStealthXPEnabled(enabled);
    return Variable();
}

Variable getStealthXPDecrement(Game &game, const vector<Variable> &args, ExecutionContext &ctx) {
    return Variable::ofInt(game.module()->area()->stealthXPDecrement());
}

Variable setStealthXPDecrement(Game &game, const vector<Variable> &args, ExecutionContext &ctx) {
    int decrement = getInt(args, 0);
    game.module()->area()->setStealthXPDecrement(decrement);
    return Variable();
}

} // namespace routine

} // namespace game

} // namespace reone
