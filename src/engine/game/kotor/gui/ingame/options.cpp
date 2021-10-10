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

#include "options.h"

#include "../../../../gui/control/button.h"

#include "../../kotor.h"

using namespace std;

using namespace reone::gui;
using namespace reone::resource;

namespace reone {

namespace game {

OptionsMenu::OptionsMenu(
    KotOR *game,
    ActionFactory &actionFactory,
    Classes &classes,
    Combat &combat,
    Feats &feats,
    FootstepSounds &footstepSounds,
    GUISounds &guiSounds,
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
    graphics::Features &features,
    graphics::Fonts &fonts,
    graphics::Lips &lips,
    graphics::Materials &materials,
    graphics::Meshes &meshes,
    graphics::Models &models,
    graphics::PBRIBL &pbrIbl,
    graphics::Shaders &shaders,
    graphics::Textures &textures,
    graphics::Walkmeshes &walkmeshes,
    graphics::Window &window,
    resource::Resources &resources,
    resource::Strings &strings) :
    GameGUI(
        game,
        actionFactory,
        classes,
        combat,
        feats,
        footstepSounds,
        guiSounds,
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
        features,
        fonts,
        lips,
        materials,
        meshes,
        models,
        pbrIbl,
        shaders,
        textures,
        walkmeshes,
        window,
        resources,
        strings) {
    _resRef = getResRef("optionsingame");

    initForGame();
    loadBackground(BackgroundType::Menu);
}

void OptionsMenu::load() {
    GUI::load();
    bindControls();

    _binding.btnLoadGame->setOnClick([this]() {
        _game->openSaveLoad(SaveLoadMode::LoadFromInGame);
    });
    _binding.btnSaveGame->setOnClick([this]() {
        _game->openSaveLoad(SaveLoadMode::Save);
    });
    _binding.btnExit->setOnClick([this]() {
        _game->openInGame();
    });
}

void OptionsMenu::bindControls() {
    _binding.btnLoadGame = getControl<Button>("BTN_LOADGAME");
    _binding.btnSaveGame = getControl<Button>("BTN_SAVEGAME");
    _binding.btnExit = getControl<Button>("BTN_EXIT");
}

} // namespace game

} // namespace reone