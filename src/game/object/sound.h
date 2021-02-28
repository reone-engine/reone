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

#include "spatial.h"

#include "../../resource/format/gfffile.h"

namespace reone {

namespace audio {

class SoundHandle;

}

namespace game {

class Sound : public SpatialObject {
public:
    Sound(
        uint32_t id,
        ObjectFactory *objectFactory,
        scene::SceneGraph *sceneGraph,
        ScriptRunner *scriptRunner);

    void update(float dt) override;

    void load(const resource::GffStruct &gffs);

    void play();
    void stop();

    bool isActive() const { return _active; }

    glm::vec3 getPosition() const;

    int priority() const { return _priority; }
    float maxDistance() const { return _maxDistance; }
    float minDistance() const { return _minDistance; }
    bool continuous() const { return _continuous; }
    float elevation() const { return _elevation; }
    bool looping() const { return _looping; }
    bool positional() const { return _positional; }

    void setAudible(bool audible);

private:
    bool _active { false };
    int _priority { 0 };
    int _soundIdx { -1 };
    float _timeout { 0.0f };
    float _maxDistance { 0.0f };
    float _minDistance { 0.0f };
    bool _continuous { false };
    float _elevation { 0.0f };
    bool _looping { false };
    bool _positional { false };
    int _interval { 0 };
    int _volume { 0 };
    std::vector<std::string> _sounds;
    bool _audible { false };
    std::shared_ptr<audio::SoundHandle> _sound;

    void loadBlueprint(const resource::GffStruct &gffs);

    void playSound(const std::string &resRef, bool loop);

    friend class SoundBlueprint;
};

} // namespace game

} // namespace reone
