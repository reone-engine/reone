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

#include "../camera.h"

namespace reone {

namespace graphics {

class OrthographicCamera : public Camera {
public:
    OrthographicCamera() :
        Camera(CameraType::Orthographic) {
    }

    void setProjection(float left, float right, float bottom, float top, float zNear, float zFar) {
        _zNear = zNear;
        _zFar = zFar;

        auto proj = glm::ortho(left, right, bottom, top, zNear, zFar);
        Camera::setProjection(proj, proj);
    }
};

} // namespace graphics

} // namespace reone
