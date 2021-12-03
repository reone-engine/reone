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

#include "context.h"

using namespace std;

namespace reone {

namespace audio {

void AudioContext::init() {
    _device = alcOpenDevice(nullptr);
    if (!_device) {
        throw runtime_error("Failed to open an OpenAL device");
    }
    _context = alcCreateContext(_device, nullptr);
    if (!_context) {
        throw runtime_error("Failed to create an OpenAL context");
    }
    alcMakeContextCurrent(_context);
}

void AudioContext::deinit() {
    if (_context) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(_context);
        _context = nullptr;
    }
    if (_device) {
        alcCloseDevice(_device);
        _device = nullptr;
    }
}

void AudioContext::setListenerPosition(glm::vec3 position) {
    if (_listenerPosition == position) {
        return;
    }
    alListener3f(AL_POSITION, position.x, position.y, position.z);
    _listenerPosition = move(position);
}

} // namespace audio

} // namespace reone
