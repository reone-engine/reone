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

#include "../control.h"

namespace reone {

namespace gui {

class Label : public Control {
public:
    Label(
        GUI &gui,
        graphics::GraphicsContext &graphicsContext,
        graphics::Fonts &fonts,
        graphics::Meshes &meshes,
        graphics::Shaders &shaders,
        graphics::Textures &textures,
        graphics::Window &window,
        resource::Strings &strings) :
        Control(
            gui,
            ControlType::Label,
            graphicsContext,
            fonts,
            meshes,
            shaders,
            textures,
            window,
            strings) {

        _focusable = false;
    }
};

} // namespace gui

} // namespace reone
