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

#include "font.h"

#include "context.h"
#include "mesh.h"
#include "meshes.h"
#include "shaders.h"
#include "texture.h"
#include "textures.h"
#include "uniforms.h"
#include "window.h"

using namespace std;

namespace reone {

namespace graphics {

void Font::load(shared_ptr<Texture> texture) {
    _texture = texture;

    const Texture::Features &features = texture->features();
    _height = features.fontHeight * 100.0f;
    _glyphs.reserve(features.numChars);

    for (int i = 0; i < features.numChars; ++i) {
        glm::vec2 ul(features.upperLeftCoords[i]);
        glm::vec2 lr(features.lowerRightCoords[i]);
        float w = lr.x - ul.x;
        float h = ul.y - lr.y;
        float aspect = w / h;

        Glyph glyph;
        glyph.ul = move(ul);
        glyph.lr = move(lr);
        glyph.size = glm::vec2(aspect * _height, _height);

        _glyphs.push_back(move(glyph));
    }
}

void Font::draw(const string &text, const glm::vec3 &position, const glm::vec3 &color, TextGravity gravity) {
    if (text.empty()) {
        return;
    }

    _shaders.use(_shaders.text());
    _textures.bind(*_texture);

    _uniforms.setGeneral([this, &color](auto &general) {
        general.resetLocals();
        general.color = glm::vec4(color, 1.0f);
    });

    int numBlocks = static_cast<int>(text.size()) / kMaxTextChars;
    if (text.size() % kMaxTextChars > 0) {
        ++numBlocks;
    }
    glm::vec3 textOffset(getTextOffset(text, gravity), 0.0f);
    for (int i = 0; i < numBlocks; ++i) {
        int numChars = glm::min(kMaxTextChars, static_cast<int>(text.size()) - i * kMaxTextChars);
        _uniforms.setText([this, &text, &position, &textOffset, &i, &numChars](auto &uniforms) {
            for (int j = 0; j < numChars; ++j) {
                const Glyph &glyph = _glyphs[static_cast<unsigned char>(text[i * kMaxTextChars + j])];

                glm::vec4 posScale;
                posScale[0] = position.x + textOffset.x;
                posScale[1] = position.y + textOffset.y;
                posScale[2] = glyph.size.x;
                posScale[3] = glyph.size.y;

                uniforms.chars[j].posScale = move(posScale);
                uniforms.chars[j].uv = glm::vec4(glyph.ul.x, glyph.lr.y, glyph.lr.x - glyph.ul.x, glyph.ul.y - glyph.lr.y);

                textOffset.x += glyph.size.x;
            }
        });
        _meshes.quad().drawInstanced(numChars);
    }
}

glm::vec2 Font::getTextOffset(const string &text, TextGravity gravity) const {
    float w = measure(text);

    switch (gravity) {
    case TextGravity::LeftCenter:
        return glm::vec2(-w, -0.5f * _height);
    case TextGravity::LeftTop:
        return glm::vec2(-w, -_height);
    case TextGravity::CenterBottom:
        return glm::vec2(-0.5f * w, 0.0f);
    case TextGravity::CenterTop:
        return glm::vec2(-0.5f * w, -_height);
    case TextGravity::RightBottom:
        return glm::vec2(0.0f, 0.0f);
    case TextGravity::RightCenter:
        return glm::vec2(0.0f, -0.5f * _height);
    case TextGravity::RightTop:
        return glm::vec2(0.0f, -_height);
    case TextGravity::CenterCenter:
    default:
        return glm::vec2(-0.5f * w, -0.5f * _height);
    }
};

float Font::measure(const string &text) const {
    float w = 0.0f;
    for (auto &glyph : text) {
        w += _glyphs[static_cast<int>(glyph)].size.x;
    }
    return w;
}

} // namespace graphics

} // namespace reone
