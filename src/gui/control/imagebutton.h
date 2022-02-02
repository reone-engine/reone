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

class ImageButton : public Control {
public:
    ImageButton(
        GUI &gui,
        scene::SceneGraphs &sceneGraphs,
        graphics::Fonts &fonts,
        graphics::GraphicsContext &graphicsContext,
        graphics::Meshes &meshes,
        graphics::Pipeline &pipeline,
        graphics::Shaders &shaders,
        graphics::Textures &textures,
        graphics::Uniforms &uniforms,
        graphics::Window &window,
        resource::Strings &strings) :
        Control(
            gui,
            ControlType::ImageButton,
            sceneGraphs,
            fonts,
            graphicsContext,
            meshes,
            pipeline,
            shaders,
            textures,
            uniforms,
            window,
            strings) {

        _clickable = true;
    }

    void load(const resource::GffStruct &gffs) override;

    void draw(
        const glm::ivec2 &offset,
        const std::vector<std::string> &text,
        const std::string &iconText,
        const std::shared_ptr<graphics::Texture> &iconTexture,
        const std::shared_ptr<graphics::Texture> &iconFrame);

private:
    std::shared_ptr<graphics::Texture> _iconFrame;
    std::shared_ptr<graphics::Font> _iconFont;

    void drawIcon(
        const glm::ivec2 &offset,
        const std::string &iconText,
        const std::shared_ptr<graphics::Texture> &iconTexture,
        const std::shared_ptr<graphics::Texture> &iconFrame);
};

} // namespace gui

} // namespace reone
