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

#include "soundsets.h"

#include <boost/algorithm/string.hpp>

#include "../audio/files.h"
#include "../common/streamutil.h"
#include "../resource/format/ssffile.h"
#include "../resource/resources.h"

using namespace std;
using namespace std::placeholders;

using namespace reone::audio;
using namespace reone::resource;

namespace reone {

namespace game {

SoundSets &SoundSets::instance() {
    static SoundSets instance;
    return instance;
}

SoundSets::SoundSets() : MemoryCache(bind(&SoundSets::doGet, this, _1)) {
}

shared_ptr<SoundSet> SoundSets::doGet(string resRef) {
    auto data = Resources::instance().get(resRef, ResourceType::Ssf);
    if (!data) return nullptr;

    auto result = make_shared<SoundSet>();

    SsfFile ssf;
    ssf.load(wrap(data));

    vector<uint32_t> sounds(ssf.soundSet());
    for (size_t i = 0; i < sounds.size(); ++i) {
        string soundResRef(boost::to_lower_copy(Resources::instance().getSoundByStrRef(sounds[i])));
        shared_ptr<AudioStream> sound(AudioFiles::instance().get(soundResRef));
        if (sound) {
            result->insert(make_pair(static_cast<SoundSetEntry>(i), sound));
        }
    }

    return move(result);
}

} // namespace game

} // namespace reone
