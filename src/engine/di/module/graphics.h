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

#include "../../graphics/context.h"
#include "../../graphics/fonts.h"
#include "../../graphics/lips.h"
#include "../../graphics/meshes.h"
#include "../../graphics/models.h"
#include "../../graphics/options.h"
#include "../../graphics/shaders.h"
#include "../../graphics/textures.h"
#include "../../graphics/walkmeshes.h"
#include "../../graphics/window.h"

namespace reone {

class ResourceModule;

class GraphicsModule : boost::noncopyable {
public:
    GraphicsModule(graphics::GraphicsOptions options, ResourceModule &resource) :
        _options(std::move(options)),
        _resource(resource) {
    }

    ~GraphicsModule() { deinit(); }

    void init();
    void deinit();

    graphics::Context &context() { return *_context; }
    graphics::Fonts &fonts() { return *_fonts; }
    graphics::Lips &lips() { return *_lips; }
    graphics::Meshes &meshes() { return *_meshes; }
    graphics::Models &models() { return *_models; }
    graphics::Shaders &shaders() { return *_shaders; }
    graphics::Textures &textures() { return *_textures; }
    graphics::Walkmeshes &walkmeshes() { return *_walkmeshes; }
    graphics::Window &window() { return *_window; }

private:
    graphics::GraphicsOptions _options;
    ResourceModule &_resource;

    std::unique_ptr<graphics::Context> _context;
    std::unique_ptr<graphics::Fonts> _fonts;
    std::unique_ptr<graphics::Lips> _lips;
    std::unique_ptr<graphics::Meshes> _meshes;
    std::unique_ptr<graphics::Models> _models;
    std::unique_ptr<graphics::Shaders> _shaders;
    std::unique_ptr<graphics::Textures> _textures;
    std::unique_ptr<graphics::Walkmeshes> _walkmeshes;
    std::unique_ptr<graphics::Window> _window;
};

} // namespace reone
