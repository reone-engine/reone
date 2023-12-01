/*
 * Copyright (c) 2020-2023 The reone project contributors
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

#include "reone/graphics/font.h"

namespace reone {

namespace graphics {

class GraphicsContext;
class Meshes;
class ShaderManager;
class Uniforms;

} // namespace graphics

namespace resource {

class Textures;

class IFonts {
public:
    virtual ~IFonts() = default;

    virtual void clear() = 0;

    virtual std::shared_ptr<graphics::Font> get(const std::string &key) = 0;
};

class Fonts : public IFonts {
public:
    Fonts(
        graphics::GraphicsContext &graphicsContext,
        graphics::Meshes &meshes,
        graphics::ShaderManager &shaderManager,
        Textures &textures,
        graphics::Uniforms &uniforms) :
        _graphicsContext(graphicsContext),
        _meshes(meshes),
        _shaderManager(shaderManager),
        _textures(textures),
        _uniforms(uniforms) {
    }

    void clear() override {
        _objects.clear();
    }

    std::shared_ptr<graphics::Font> get(const std::string &key) override {
        auto maybeObject = _objects.find(key);
        if (maybeObject != _objects.end()) {
            return maybeObject->second;
        }
        auto object = doGet(key);
        return _objects.insert(make_pair(key, std::move(object))).first->second;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<graphics::Font>> _objects;

    // Services

    graphics::GraphicsContext &_graphicsContext;
    graphics::Meshes &_meshes;
    graphics::ShaderManager &_shaderManager;
    Textures &_textures;
    graphics::Uniforms &_uniforms;

    // END Services

    std::shared_ptr<graphics::Font> doGet(std::string resRef);
};

} // namespace resource

} // namespace reone
