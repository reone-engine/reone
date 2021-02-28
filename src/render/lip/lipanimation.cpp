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

#include "lipanimation.h"

#include <stdexcept>

using namespace std;

namespace reone {

namespace render {

LipAnimation::LipAnimation(float length, vector<Keyframe> keyframes) :
    _length(length), _keyframes(move(keyframes)) {
}

bool LipAnimation::getKeyframes(float time, uint8_t &leftShape, uint8_t &rightShape, float &interpolant) const {
    if (_keyframes.empty()) return false;

    const Keyframe *left = &_keyframes[0];
    const Keyframe *right = &_keyframes[0];

    for (auto &frame : _keyframes) {
        if (time > frame.time) {
            left = &frame;
        }
        if (time <= frame.time) {
            right = &frame;
            break;
        }
    }

    leftShape = left->shape;
    rightShape = right->shape;

    if (&left == &right) {
        interpolant = 0.0f;
    } else {
        interpolant = (time - left->time) / (right->time - left->time);
    }

    return true;
}

} // namespace render

} // namespace reone
