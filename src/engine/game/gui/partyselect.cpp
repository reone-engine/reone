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

#include "partyselect.h"

#include "../../graphics/texture/textures.h"
#include "../../resource/resources.h"
#include "../../resource/strings.h"
#include "../../script/types.h"

#include "../game.h"
#include "../object/factory.h"
#include "../party.h"
#include "../portrait.h"
#include "../portraits.h"
#include "../script/runner.h"

using namespace std;

using namespace reone::gui;
using namespace reone::graphics;
using namespace reone::resource;
using namespace reone::script;

namespace reone {

namespace game {

static constexpr int kMaxFollowerCount = 2;

static int g_strRefAdd = 38455;
static int g_strRefRemove = 38456;

static glm::vec3 g_kotorColorOn = {0.984314f, 1.0f, 0};
static glm::vec3 g_kotorColorAdded = {0, 0.831373f, 0.090196f};

PartySelection::PartySelection(
    Game *game,
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
    if (game->isTSL()) {
        _resRef = "partyselect_p";
    } else {
        _resRef = "partyselection";
        loadBackground(BackgroundType::Menu);
    }

    initForGame();
}

void PartySelection::load() {
    GUI::load();
    bindControls();

    for (int i = 0; i < kNpcCount; ++i) {
        ToggleButton &button = getNpcButton(i);
        button.setOnColor(g_kotorColorOn);
        button.setBorderColorOverride(g_kotorColorAdded);
        button.setUseBorderColorOverride(false);
    }

    _binding.btnAccept->setOnClick([this]() {
        onAcceptButtonClick();
    });
    _binding.btnDone->setOnClick([this]() {
        changeParty();
        _game->openInGame();
        if (!_context.exitScript.empty()) {
            _scriptRunner.run(_context.exitScript);
        }
    });
    _binding.btnBack->setOnClick([this]() {
        _game->openInGame();
        if (!_context.exitScript.empty()) {
            _scriptRunner.run(_context.exitScript);
        }
    });
    _binding.btnNpc0->setOnClick([this]() {
        onNpcButtonClick(0);
    });
    _binding.btnNpc1->setOnClick([this]() {
        onNpcButtonClick(1);
    });
    _binding.btnNpc2->setOnClick([this]() {
        onNpcButtonClick(2);
    });
    _binding.btnNpc3->setOnClick([this]() {
        onNpcButtonClick(3);
    });
    _binding.btnNpc4->setOnClick([this]() {
        onNpcButtonClick(4);
    });
    _binding.btnNpc5->setOnClick([this]() {
        onNpcButtonClick(5);
    });
    _binding.btnNpc6->setOnClick([this]() {
        onNpcButtonClick(6);
    });
    _binding.btnNpc7->setOnClick([this]() {
        onNpcButtonClick(7);
    });
    _binding.btnNpc8->setOnClick([this]() {
        onNpcButtonClick(8);
    });

    if (_game->isTSL()) {
        _binding.btnNpc9->setOnClick([this]() {
            onNpcButtonClick(9);
        });
        _binding.btnNpc10->setOnClick([this]() {
            onNpcButtonClick(10);
        });
        _binding.btnNpc11->setOnClick([this]() {
            onNpcButtonClick(11);
        });
    }
}

void PartySelection::bindControls() {
    _binding.btnAccept = getControl<Button>("BTN_ACCEPT");
    _binding.btnBack = getControl<Button>("BTN_BACK");
    _binding.btnDone = getControl<Button>("BTN_DONE");
    _binding.btnNpc0 = getControl<ToggleButton>("BTN_NPC0");
    _binding.btnNpc1 = getControl<ToggleButton>("BTN_NPC1");
    _binding.btnNpc2 = getControl<ToggleButton>("BTN_NPC2");
    _binding.btnNpc3 = getControl<ToggleButton>("BTN_NPC3");
    _binding.btnNpc4 = getControl<ToggleButton>("BTN_NPC4");
    _binding.btnNpc5 = getControl<ToggleButton>("BTN_NPC5");
    _binding.btnNpc6 = getControl<ToggleButton>("BTN_NPC6");
    _binding.btnNpc7 = getControl<ToggleButton>("BTN_NPC7");
    _binding.btnNpc8 = getControl<ToggleButton>("BTN_NPC8");
    _binding.lbl3d = getControl<Label>("LBL_3D");
    _binding.lblBevelL = getControl<Label>("LBL_BEVEL_L");
    _binding.lblBevelM = getControl<Label>("LBL_BEVEL_M");
    _binding.lblChar0 = getControl<Label>("LBL_CHAR0");
    _binding.lblChar1 = getControl<Label>("LBL_CHAR1");
    _binding.lblChar2 = getControl<Label>("LBL_CHAR2");
    _binding.lblChar3 = getControl<Label>("LBL_CHAR3");
    _binding.lblChar4 = getControl<Label>("LBL_CHAR4");
    _binding.lblChar5 = getControl<Label>("LBL_CHAR5");
    _binding.lblChar6 = getControl<Label>("LBL_CHAR6");
    _binding.lblChar7 = getControl<Label>("LBL_CHAR7");
    _binding.lblChar8 = getControl<Label>("LBL_CHAR8");
    _binding.lblCount = getControl<Label>("LBL_COUNT");
    _binding.lblNa0 = getControl<Label>("LBL_NA0");
    _binding.lblNa1 = getControl<Label>("LBL_NA1");
    _binding.lblNa2 = getControl<Label>("LBL_NA2");
    _binding.lblNa3 = getControl<Label>("LBL_NA3");
    _binding.lblNa4 = getControl<Label>("LBL_NA4");
    _binding.lblNa5 = getControl<Label>("LBL_NA5");
    _binding.lblNa6 = getControl<Label>("LBL_NA6");
    _binding.lblNa7 = getControl<Label>("LBL_NA7");
    _binding.lblNa8 = getControl<Label>("LBL_NA8");
    _binding.lblNpcLevel = getControl<Label>("LBL_NPC_LEVEL");
    _binding.lblNpcName = getControl<Label>("LBL_NPC_NAME");
    _binding.lblTitle = getControl<Label>("LBL_TITLE");

    if (_game->isTSL()) {
        _binding.btnNpc9 = getControl<ToggleButton>("BTN_NPC9");
        _binding.btnNpc10 = getControl<ToggleButton>("BTN_NPC10");
        _binding.btnNpc11 = getControl<ToggleButton>("BTN_NPC11");
        _binding.lblChar9 = getControl<Label>("LBL_CHAR9");
        _binding.lblChar10 = getControl<Label>("LBL_CHAR10");
        _binding.lblChar11 = getControl<Label>("LBL_CHAR11");
        _binding.lblNa9 = getControl<Label>("LBL_NA9");
        _binding.lblNa10 = getControl<Label>("LBL_NA10");
        _binding.lblNa11 = getControl<Label>("LBL_NA11");
        _binding.lblNameBack = getControl<Label>("LBL_NAMEBACK");
    } else {
        _binding.lblAvailable = getControl<Label>("LBL_AVAILABLE");
        _binding.lblBevelR = getControl<Label>("LBL_BEVEL_R");
    }
}

void PartySelection::prepare(const Context &ctx) {
    _context = ctx;
    _availableCount = kMaxFollowerCount;

    for (int i = 0; i < kNpcCount; ++i) {
        _added[i] = false;
        getNpcButton(i).setUseBorderColorOverride(false);
    }
    if (ctx.forceNpc1 >= 0) {
        addNpc(ctx.forceNpc1);
    }
    if (ctx.forceNpc2 >= 0) {
        addNpc(ctx.forceNpc2);
    }
    Party &party = _party;
    vector<Label *> charLabels {
        _binding.lblChar0.get(),
        _binding.lblChar1.get(),
        _binding.lblChar2.get(),
        _binding.lblChar3.get(),
        _binding.lblChar4.get(),
        _binding.lblChar5.get(),
        _binding.lblChar6.get(),
        _binding.lblChar7.get(),
        _binding.lblChar8.get(),
    };
    vector<Label *> naLabels {
        _binding.lblNa0.get(),
        _binding.lblNa1.get(),
        _binding.lblNa2.get(),
        _binding.lblNa3.get(),
        _binding.lblNa4.get(),
        _binding.lblNa5.get(),
        _binding.lblNa6.get(),
        _binding.lblNa7.get(),
        _binding.lblNa8.get()};
    if (_game->isTSL()) {
        charLabels.push_back(_binding.lblChar9.get());
        charLabels.push_back(_binding.lblChar10.get());
        charLabels.push_back(_binding.lblChar11.get());
        naLabels.push_back(_binding.lblNa9.get());
        naLabels.push_back(_binding.lblNa10.get());
        naLabels.push_back(_binding.lblNa11.get());
    }

    for (int i = 0; i < kNpcCount; ++i) {
        ToggleButton &btnNpc = getNpcButton(i);
        Label &lblChar = *charLabels[i];
        Label &lblNa = *naLabels[i];

        if (party.isMemberAvailable(i)) {
            string blueprintResRef(party.getAvailableMember(i));
            shared_ptr<GffStruct> utc(_resources.getGFF(blueprintResRef, ResourceType::Utc));
            shared_ptr<Texture> portrait;
            int portraitId = utc->getInt("PortraitId", 0);
            if (portraitId > 0) {
                portrait = _portraits.getTextureByIndex(portraitId);
            } else {
                int appearance = utc->getInt("Appearance_Type");
                portrait = _portraits.getTextureByAppearance(appearance);
            }
            btnNpc.setDisabled(false);
            lblChar.setBorderFill(move(portrait));
            lblNa.setVisible(false);
        } else {
            btnNpc.setDisabled(true);
            lblChar.setBorderFill(shared_ptr<Texture>(nullptr));
            lblNa.setVisible(true);
        }
    }
    refreshAvailableCount();
}

void PartySelection::addNpc(int npc) {
    --_availableCount;
    _added[npc] = true;
    getNpcButton(npc).setUseBorderColorOverride(true);
    refreshAvailableCount();
}

ToggleButton &PartySelection::getNpcButton(int npc) {
    vector<ToggleButton *> npcButtons {
        _binding.btnNpc0.get(),
        _binding.btnNpc1.get(),
        _binding.btnNpc2.get(),
        _binding.btnNpc3.get(),
        _binding.btnNpc4.get(),
        _binding.btnNpc5.get(),
        _binding.btnNpc6.get(),
        _binding.btnNpc7.get(),
        _binding.btnNpc8.get(),
        _binding.btnNpc9.get()};
    if (_game->isTSL()) {
        npcButtons.push_back(_binding.btnNpc10.get());
        npcButtons.push_back(_binding.btnNpc11.get());
    }
    return *npcButtons[npc];
}

void PartySelection::onAcceptButtonClick() {
    if (_selectedNpc == -1)
        return;

    bool added = _added[_selectedNpc];
    if (added && _context.forceNpc1 != _selectedNpc && _context.forceNpc2 != _selectedNpc) {
        removeNpc(_selectedNpc);
        refreshAcceptButton();

    } else if (!added && _availableCount > 0) {
        addNpc(_selectedNpc);
        refreshAcceptButton();
    }
}

void PartySelection::refreshAvailableCount() {
    _binding.lblCount->setTextMessage(to_string(_availableCount));
}

void PartySelection::refreshAcceptButton() {
    string text(_strings.get(_added[_selectedNpc] ? g_strRefRemove : g_strRefAdd));
    _binding.btnAccept->setTextMessage(text);
}

void PartySelection::removeNpc(int npc) {
    ++_availableCount;
    _added[npc] = false;
    getNpcButton(npc).setUseBorderColorOverride(false);
    refreshAvailableCount();
}

void PartySelection::onNpcButtonClick(int npc) {
    _selectedNpc = npc;
    refreshNpcButtons();
    refreshAcceptButton();
}

void PartySelection::refreshNpcButtons() {
    for (int i = 0; i < kNpcCount; ++i) {
        ToggleButton &button = getNpcButton(i);

        if ((i == _selectedNpc && !button.isOn()) ||
            (i != _selectedNpc && button.isOn())) {

            button.toggle();
        }
    }
}

void PartySelection::changeParty() {
    shared_ptr<Area> area(_game->module()->area());
    area->unloadParty();

    Party &party = _party;
    party.clear();
    party.addMember(kNpcPlayer, party.player());

    shared_ptr<Creature> player(_party.player());

    for (int i = 0; i < kNpcCount; ++i) {
        if (!_added[i])
            continue;

        string blueprintResRef(party.getAvailableMember(i));

        shared_ptr<Creature> creature(_objectFactory.newCreature());
        creature->loadFromBlueprint(blueprintResRef);
        creature->setFaction(Faction::Friendly1);
        creature->setImmortal(true);
        party.addMember(i, creature);
    }

    area->reloadParty();
}

} // namespace game

} // namespace reone
