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

#include "control.h"

#include "../../common/logutil.h"
#include "../../graphics/context.h"
#include "../../graphics/fonts.h"
#include "../../graphics/meshes.h"
#include "../../graphics/pipeline.h"
#include "../../graphics/services.h"
#include "../../graphics/shaders.h"
#include "../../graphics/texture.h"
#include "../../graphics/textures.h"
#include "../../graphics/textureutil.h"
#include "../../graphics/uniforms.h"
#include "../../graphics/window.h"
#include "../../resource/gff.h"
#include "../../resource/services.h"
#include "../../resource/strings.h"
#include "../../scene/graph.h"

using namespace std;

using namespace reone::graphics;
using namespace reone::resource;

namespace reone {

namespace gui {

namespace neo {

void Control::load(const Gff &gui, const glm::vec4 &scale) {
    _tag = gui.getString("TAG");

    auto extent = gui.getStruct("EXTENT");
    if (extent) {
        _extent = glm::ivec4(scale * glm::vec4(
                                         extent->getInt("LEFT"),
                                         extent->getInt("TOP"),
                                         extent->getInt("WIDTH"),
                                         extent->getInt("HEIGHT")));
    }

    auto border = gui.getStruct("BORDER");
    if (border) {
        _border = make_unique<Border>(Border {
            border->getString("CORNER"),
            border->getString("EDGE"),
            border->getString("FILL"),
            border->getInt("DIMENSION"),
            border->getVector("COLOR")});
    }

    auto hilight = gui.getStruct("HILIGHT");
    if (hilight) {
        _hilight = make_unique<Border>(Border {
            hilight->getString("CORNER"),
            hilight->getString("EDGE"),
            hilight->getString("FILL"),
            hilight->getInt("DIMENSION"),
            hilight->getVector("COLOR")});
    }

    auto text = gui.getStruct("TEXT");
    if (text) {
        _text = make_unique<Text>(Text {
            static_cast<TextAlignment>(text->getInt("ALIGNMENT")),
            text->getVector("COLOR"),
            text->getString("FONT"),
            text->getString("TEXT"),
            text->getInt("STRREF")});
    }
}

void Control::update(float delta) {
    if (!_enabled) {
        return;
    }
    if (_sceneGraph) {
        _sceneGraph->update(delta);
    }
    for (auto &child : _children) {
        child->update(delta);
    }
}

void Control::render() {
    if (!_enabled) {
        return;
    }

    auto &border = (_focus && _hilight) ? _hilight : _border;

    // Render fill
    if (border && !border->fill.empty()) {
        _graphicsSvc.shaders.use(_graphicsSvc.shaders.gui());

        auto fillTexture = _graphicsSvc.textures.get(border->fill, TextureUsage::GUI);
        _graphicsSvc.textures.bind(*fillTexture);

        _graphicsSvc.uniforms.setGeneral([this, &border](auto &u) {
            u.resetLocals();
            u.model = glm::translate(glm::vec3(static_cast<float>(_extent[0]), static_cast<float>(_extent[1]), 0.0f));
            u.model *= glm::scale(glm::vec3(static_cast<float>(_extent[2]), static_cast<float>(_extent[3]), 1.0f));
            if (_flipVertical) {
                u.uv = glm::mat3x4(
                    glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
                    glm::vec4(0.0f, -1.0f, 0.0f, 0.0f),
                    glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
            }
            u.color = glm::vec4(border->color, 1.0f);
        });

        auto blendMode = hasAlphaChannel(fillTexture->pixelFormat()) ? BlendMode::Normal : BlendMode::Additive;
        _graphicsSvc.context.withBlending(blendMode, [this]() {
            _graphicsSvc.meshes.quad().draw();
        });
    }

    // Render 3D scene
    if (_sceneGraph) {
        auto output = _graphicsSvc.pipeline.draw(*_sceneGraph, glm::ivec2(_extent[2], _extent[3]));
        if (output) {
            _graphicsSvc.textures.bind(*output);
            _graphicsSvc.uniforms.setGeneral([this](auto &u) {
                u.resetGlobals();
                u.resetLocals();
                u.projection = _graphicsSvc.window.getOrthoProjection();
                u.model = glm::translate(glm::vec3(static_cast<float>(_extent[0]), static_cast<float>(_extent[1]), 0.0f));
                u.model *= glm::scale(glm::vec3(static_cast<float>(_extent[2]), static_cast<float>(_extent[3]), 1.0f));
            });
            _graphicsSvc.shaders.use(_graphicsSvc.shaders.gui());
            _graphicsSvc.context.withBlending(BlendMode::Normal, [this]() {
                _graphicsSvc.context.withDepthTest(DepthTestMode::None, [this]() {
                    _graphicsSvc.meshes.quad().draw();
                });
            });
        }
    }

    // Render corners
    if (border && !border->corner.empty()) {
        _graphicsSvc.shaders.use(_graphicsSvc.shaders.gui());

        auto cornerTexture = _graphicsSvc.textures.get(border->corner, TextureUsage::GUI);
        _graphicsSvc.textures.bind(*cornerTexture);

        auto topLeftModel = glm::translate(glm::vec3(static_cast<float>(_extent[0]), static_cast<float>(_extent[1]), 0.0f));
        topLeftModel *= glm::scale(glm::vec3(border->dimension, border->dimension, 1.0f));
        auto topLeftUv = glm::mat3x4(1.0f);

        auto topRightModel = glm::translate(glm::vec3(static_cast<float>(_extent[0] + _extent[2] - border->dimension), static_cast<float>(_extent[1]), 0.0f));
        topRightModel *= glm::scale(glm::vec3(border->dimension, border->dimension, 1.0f));
        auto topRightUv = glm::mat3x4(
            glm::vec4(-1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
            glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));

        auto bottomLeftModel = glm::translate(glm::vec3(static_cast<float>(_extent[0]), static_cast<float>(_extent[1] + _extent[3] - border->dimension), 0.0f));
        bottomLeftModel *= glm::scale(glm::vec3(border->dimension, border->dimension, 1.0f));
        auto bottomLeftUv = glm::mat3x4(
            glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, -1.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));

        auto bottomRightModel = glm::translate(glm::vec3(static_cast<float>(_extent[0] + _extent[2] - border->dimension), static_cast<float>(_extent[1] + _extent[3] - border->dimension), 0.0f));
        bottomRightModel *= glm::scale(glm::vec3(border->dimension, border->dimension, 1.0f));
        auto bottomRightUv = glm::mat3x4(
            glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
            glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

        vector<pair<glm::mat4, glm::mat4>> edges {
            make_pair(topLeftModel, topLeftUv),
            make_pair(topRightModel, topRightUv),
            make_pair(bottomLeftModel, bottomLeftUv),
            make_pair(bottomRightModel, bottomRightUv)};

        auto blendMode = hasAlphaChannel(cornerTexture->pixelFormat()) ? BlendMode::Normal : BlendMode::Additive;
        _graphicsSvc.context.withBlending(blendMode, [&]() {
            for (auto &edge : edges) {
                _graphicsSvc.uniforms.setGeneral([&](auto &u) {
                    u.resetLocals();
                    u.model = edge.first;
                    u.uv = edge.second;
                    u.color = glm::vec4(border->color, 1.0f);
                });
                _graphicsSvc.meshes.quad().draw();
            }
        });
    }

    // Render edges
    if (border && !border->edge.empty()) {
        _graphicsSvc.shaders.use(_graphicsSvc.shaders.gui());

        auto edgeTexture = _graphicsSvc.textures.get(border->edge, TextureUsage::GUI);
        _graphicsSvc.textures.bind(*edgeTexture);

        auto leftModel = glm::translate(glm::vec3(static_cast<float>(_extent[0]), static_cast<float>(_extent[1] + border->dimension), 0.0f));
        leftModel *= glm::scale(glm::vec3(border->dimension, static_cast<float>(_extent[3] - 2 * border->dimension), 1.0f));
        auto leftUv = glm::mat3x4(
            glm::vec4(0.0f, -1.0f, 0.0f, 0.0f),
            glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));

        auto topModel = glm::translate(glm::vec3(static_cast<float>(_extent[0] + border->dimension), static_cast<float>(_extent[1]), 0.0f));
        topModel *= glm::scale(glm::vec3(static_cast<float>(_extent[2] - 2 * border->dimension), border->dimension, 1.0f));
        auto topUv = glm::mat3x4(1.0f);

        auto rightModel = glm::translate(glm::vec3(static_cast<float>(_extent[0] + _extent[2] - border->dimension), static_cast<float>(_extent[1] + border->dimension), 0.0f));
        rightModel *= glm::scale(glm::vec3(border->dimension, static_cast<float>(_extent[3] - 2 * border->dimension), 1.0f));
        auto rightUv = glm::mat3x4(
            glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
            glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

        auto bottomModel = glm::translate(glm::vec3(static_cast<float>(_extent[0] + border->dimension), static_cast<float>(_extent[1] + _extent[3] - border->dimension), 0.0f));
        bottomModel *= glm::scale(glm::vec3(static_cast<float>(_extent[2] - 2 * border->dimension), border->dimension, 1.0f));
        auto bottomUv = glm::mat3x4(
            glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, -1.0f, 0.0f, 0.0f),
            glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));

        vector<pair<glm::mat4, glm::mat4>> edges {
            make_pair(leftModel, leftUv),
            make_pair(topModel, topUv),
            make_pair(rightModel, rightUv),
            make_pair(bottomModel, bottomUv)};

        auto blendMode = hasAlphaChannel(edgeTexture->pixelFormat()) ? BlendMode::Normal : BlendMode::Additive;
        _graphicsSvc.context.withBlending(blendMode, [&]() {
            for (auto &edge : edges) {
                _graphicsSvc.uniforms.setGeneral([&](auto &u) {
                    u.resetLocals();
                    u.model = edge.first;
                    u.uv = edge.second;
                    u.color = glm::vec4(border->color, 1.0f);
                });
                _graphicsSvc.meshes.quad().draw();
            }
        });
    }

    // Render text
    if (_text && (!_text->text.empty() || _text->strref != -1)) {
        auto font = _graphicsSvc.fonts.get(_text->font);
        string text = !_text->text.empty() ? _text->text : _resourceSvc.strings.get(_text->strref);

        glm::ivec2 position;
        TextGravity gravity;
        getTextPlacement(position, gravity);

        _graphicsSvc.context.withBlending(BlendMode::Normal, [&]() {
            font->draw(text, glm::vec3(position, 1.0f), _text->color, gravity);
        });
    }

    // Render children
    for (auto &child : _children) {
        child->render();
    }
}

bool Control::isInExtent(float x, float y) const {
    return _extent[0] <= x && x <= _extent[0] + _extent[2] &&
           _extent[1] <= y && y <= _extent[1] + _extent[3];
}

void Control::getTextPlacement(glm::ivec2 &outPosition, TextGravity &outGravity) const {
    switch (_text->alignment) {
    case TextAlignment::LeftTop:
        outPosition = glm::ivec2(_extent[0], _extent[1]);
        outGravity = TextGravity::RightBottom;
        break;
    case TextAlignment::CenterTop:
        outPosition = glm::ivec2(_extent[0] + _extent[2] / 2, _extent[1]);
        outGravity = TextGravity::CenterBottom;
        break;
    case TextAlignment::CenterCenter:
        outPosition = glm::ivec2(_extent[0] + _extent[2] / 2, _extent[1] + _extent[3] / 2);
        outGravity = TextGravity::CenterCenter;
        break;
    case TextAlignment::RightCenter:
    case TextAlignment::RightCenter2:
        outPosition = glm::ivec2(_extent[0] + _extent[2], _extent[1] + _extent[3] / 2);
        outGravity = TextGravity::LeftCenter;
        break;
    case TextAlignment::LeftCenter:
        outPosition = glm::ivec2(_extent[0], _extent[1] + _extent[3] / 2);
        outGravity = TextGravity::RightCenter;
        break;
    case TextAlignment::CenterBottom:
        outPosition = glm::ivec2(_extent[0] + _extent[2] / 2, _extent[1] + _extent[3]);
        outGravity = TextGravity::CenterTop;
        break;
    default:
        throw invalid_argument("Unsupported text alignment: " + to_string(static_cast<int>(_text->alignment)));
    }
}

Control *Control::findControlByTag(const string &tag) {
    if (_tag == tag) {
        return this;
    }
    for (auto &child : _children) {
        auto control = child->findControlByTag(tag);
        if (control) {
            return control;
        }
    }
    return nullptr;
}

Control *Control::pickControlAt(int x, int y) {
    if (!_enabled) {
        return nullptr;
    }
    for (auto &child : _children) {
        auto control = child->pickControlAt(x, y);
        if (control) {
            return control;
        }
    }
    if (_clickable && isInExtent(x, y)) {
        return this;
    }
    return nullptr;
}

} // namespace neo

} // namespace gui

} // namespace reone
