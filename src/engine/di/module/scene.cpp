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

#include "scene.h"

#include "../../game/core/types.h"

#include "graphics.h"

using namespace std;

using namespace reone::game;
using namespace reone::graphics;
using namespace reone::scene;

namespace reone {

namespace di {

void SceneModule::init() {
    _sceneGraphs = make_unique<SceneGraphs>(
        _options,
        _graphics.context(),
        _graphics.features(),
        _graphics.materials(),
        _graphics.meshes(),
        _graphics.pbrIbl(),
        _graphics.shaders(),
        _graphics.textures());

    auto &mainGraph = _sceneGraphs->get(kSceneNameMain);
    _worldRenderPipeline = make_unique<WorldRenderPipeline>(_options, mainGraph, _graphics.context(), _graphics.meshes(), _graphics.shaders());
    _worldRenderPipeline->init();

    _controlRenderPipeline = make_unique<ControlRenderPipeline>(*_sceneGraphs, _graphics.context(), _graphics.meshes(), _graphics.shaders());
    _controlRenderPipeline->init();
}

} // namespace di

} // namespace reone
