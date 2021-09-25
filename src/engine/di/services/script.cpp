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

#include "script.h"

#include "resource.h"

using namespace std;

using namespace reone::resource;
using namespace reone::script;

namespace reone {

namespace di {

ScriptServices::ScriptServices(ResourceServices &resource) : _resource(resource) {
}

void ScriptServices::init() {
    _scripts = make_unique<Scripts>(_resource.resources());
}

} // namespace di

} // namespace reone
