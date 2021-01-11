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

#include "texture.h"

namespace reone {

namespace render {

class Framebuffer {
public:
    Framebuffer(int w, int h, int colorBufferCount = 1, bool cubeMapDepthBuffer = false);
    ~Framebuffer();

    void init();
    void deinit();

    void bind() const;
    void unbind() const;

    void bindColorBuffer(int n) const;
    void unbindColorBuffer(int n) const;

    void bindDepthBuffer() const;
    void unbindDepthBuffer() const;

    int width() const;
    int height() const;

private:
    int _width { 0 };
    int _height { 0 };
    int _numColorBuffers { 0 };
    bool _cubeMapDepthBuffer { false };

    bool _inited { false };
    uint32_t _framebuffer { 0 };
    std::vector<std::unique_ptr<Texture>> _colorBuffers;
    std::unique_ptr<Texture> _depthBuffer;

    Framebuffer(const Framebuffer &) = delete;
    Framebuffer &operator=(const Framebuffer &) = delete;
};

} // namespace render

} // namespace reone
