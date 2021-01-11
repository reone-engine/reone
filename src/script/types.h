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

#include <cstdint>
#include <memory>
#include <vector>

namespace reone {

namespace script {

constexpr uint32_t kObjectSelf = 0;
constexpr uint32_t kObjectInvalid = 1;

struct Variable;

class ScriptObject;
class ScriptProgram;
class Routine;

struct ExecutionState {
    std::shared_ptr<ScriptProgram> program;
    std::vector<Variable> globals;
    std::vector<Variable> locals;
    uint32_t insOffset { 0 };
};

class IRoutineProvider {
public:
    virtual ~IRoutineProvider() {
    }

    virtual const Routine &get(int index) = 0;
};

struct ExecutionContext {
    IRoutineProvider *routines { nullptr };
    std::shared_ptr<ExecutionState> savedState;
    std::shared_ptr<ScriptObject> caller;
    std::shared_ptr<ScriptObject> triggerer;
    int userDefinedEventNumber { -1 };
};

} // namespace script

} // namespace reone
