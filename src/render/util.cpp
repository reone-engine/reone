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

#include "util.h"

#include "GL/glew.h"

#include "SDL2/SDL_opengl.h"

using namespace std;

namespace reone {

namespace render {

void withDepthTest(const function<void()> &block) {
    glEnable(GL_DEPTH_TEST);
    block();
    glDisable(GL_DEPTH_TEST);
}

void withAdditiveBlending(const function<void()> &block) {
    GLint blendSrcRgb, blendSrcAlpha, blendDstRgb, blendDstAlpha;
    glGetIntegerv(GL_BLEND_SRC_RGB, &blendSrcRgb);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_RGB, &blendDstRgb);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &blendDstAlpha);
    glBlendFunc(GL_ONE, GL_ONE);

    block();

    glBlendFuncSeparate(blendSrcRgb, blendDstRgb, blendSrcAlpha, blendDstAlpha);
}

void setActiveTextureUnit(int n) {
    glActiveTexture(GL_TEXTURE0 + n);
}

} // namespace render

} // namespace reone
