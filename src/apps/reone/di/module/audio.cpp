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

#include "audio.h"

#include "resource.h"

using namespace std;

using namespace reone::audio;

namespace reone {

namespace engine {

void AudioModule::init() {
    _audioFiles = make_unique<AudioFiles>(_resource.resources());
    _audioContext = make_unique<AudioContext>();
    _audioPlayer = make_unique<AudioPlayer>(_options, *_audioFiles);

    _services = make_unique<AudioServices>(*_audioContext, *_audioFiles, *_audioPlayer);

    _audioContext->init();
}

void AudioModule::deinit() {
    _services.reset();

    _audioPlayer.reset();
    _audioContext.reset();
    _audioFiles.reset();
}

} // namespace engine

} // namespace reone