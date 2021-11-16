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

#include "../../../audio/soundhandle.h"
#include "../../../resource/format/gffreader.h"

namespace reone {

namespace game {

class Game;

class Sound : public SpatialObject {
public:
    Sound(
        uint32_t id,
        Game *game,
        ActionFactory &actionFactory,
        Classes &classes,
        Combat &combat,
        FootstepSounds &footstepSounds,
        ObjectFactory &objectFactory,
        Party &party,
        Portraits &portraits,
        Reputes &reputes,
        ScriptRunner &scriptRunner,
        SoundSets &soundSets,
        Surfaces &surfaces,
        audio::AudioFiles &audioFiles,
        audio::AudioPlayer &audioPlayer,
        graphics::Context &context,
        graphics::Meshes &meshes,
        graphics::Models &models,
        graphics::Shaders &shaders,
        graphics::Textures &textures,
        graphics::Walkmeshes &walkmeshes,
        resource::Gffs &gffs,
        resource::Resources &resources,
        resource::Strings &strings,
        resource::TwoDas &twoDas,
        scene::SceneGraph &sceneGraph) :
        SpatialObject(
            id,
            ObjectType::Sound,
            game,
            actionFactory,
            classes,
            combat,
            footstepSounds,
            objectFactory,
            party,
            portraits,
            reputes,
            scriptRunner,
            soundSets,
            surfaces,
            audioFiles,
            audioPlayer,
            context,
            meshes,
            models,
            shaders,
            textures,
            walkmeshes,
            gffs,
            resources,
            strings,
            twoDas,
            sceneGraph) {
    }

    void loadFromGIT(const resource::GffStruct &gffs);
    void loadFromBlueprint(const std::string &resRef);

    void update(float dt) override;

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
    bool _active {false};
    int _priority {0};
    int _soundIdx {-1};
    float _timeout {0.0f};
    float _maxDistance {0.0f};
    float _minDistance {0.0f};
    bool _continuous {false};
    float _elevation {0.0f};
    bool _looping {false};
    bool _positional {false};
    int _interval {0};
    int _volume {0};
    std::vector<std::string> _sounds;
    bool _audible {false};
    std::shared_ptr<audio::SoundHandle> _sound;
    bool _randomPosition {false};
    int _random {0};
    float _randomRangeX {0.0f};
    float _randomRangeY {0.0f};
    int _intervalVrtn {0};
    float _pitchVariation {0.0f};
    int _volumeVrtn {0};

    void loadTransformFromGIT(const resource::GffStruct &gffs);

    void playSound(const std::string &resRef, bool loop);

    // Blueprint

    void loadUTS(const resource::GffStruct &uts);

    void loadPriorityFromUTS(const resource::GffStruct &uts);

    // END Blueprint
};

} // namespace game

} // namespace reone
