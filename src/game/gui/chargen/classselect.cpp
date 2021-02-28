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

#include "classselect.h"

#include "../../../gui/scenebuilder.h"
#include "../../../render/model/models.h"
#include "../../../resource/resources.h"

#include "../../characterutil.h"
#include "../../game.h"
#include "../../object/creature.h"
#include "../../rp/classes.h"

#include "../colorutil.h"

#include "chargen.h"

using namespace std;

using namespace reone::gui;
using namespace reone::render;
using namespace reone::resource;
using namespace reone::scene;

namespace reone {

namespace game {

static constexpr float kModelScale = 1.1f;

static map<Gender, int> g_genderStrRefs {
    { Gender::Male, 646 },
    { Gender::Female, 647 }
};

static map<ClassType, int> g_classDescStrRefs {
    { ClassType::Scoundrel, 32109 },
    { ClassType::Scout, 32110 },
    { ClassType::Soldier, 32111 },
    { ClassType::JediConsular, 48031 },
    { ClassType::JediSentinel, 48032 },
    { ClassType::JediGuardian, 48033 }
};

ClassSelection::ClassSelection(Game *game) :
    GameGUI(game->gameId(), game->options().graphics),
    _game(game) {

    _resRef = getResRef("classsel");

    if (_gameId == GameID::KotOR) {
        _backgroundType = BackgroundType::Menu;
    }

    initForGame();
}

void ClassSelection::load() {
    GUI::load();
    configureClassButtons();
    configureClassModels();

    Control &backButton = getControl("BTN_BACK");
    setButtonColors(backButton);
}

void ClassSelection::configureClassButtons() {
    int x, y;

    Control &button1 = getControl("BTN_SEL1");
    setButtonColors(button1);
    button1.extent().getCenter(x, y);
    _classButtons.push_back({ &button1, glm::vec2(x, y), randomCharacter(Gender::Male, _gameId == GameID::KotOR ? ClassType::Scoundrel : ClassType::JediConsular) });

    Control &button2 = getControl("BTN_SEL2");
    setButtonColors(button2);
    button2.extent().getCenter(x, y);
    _classButtons.push_back({ &button2, glm::vec2(x, y), randomCharacter(Gender::Male, _gameId == GameID::KotOR ? ClassType::Scout : ClassType::JediSentinel) });

    Control &button3 = getControl("BTN_SEL3");
    setButtonColors(button3);
    button3.extent().getCenter(x, y);
    _classButtons.push_back({ &button3, glm::vec2(x, y), randomCharacter(Gender::Male, _gameId == GameID::KotOR ? ClassType::Soldier : ClassType::JediGuardian) });

    Control &button4 = getControl("BTN_SEL4");
    setButtonColors(button4);
    button4.extent().getCenter(x, y);
    _classButtons.push_back({ &button4, glm::vec2(x, y), randomCharacter(Gender::Female, _gameId == GameID::KotOR ? ClassType::Soldier : ClassType::JediGuardian) });

    Control &button5 = getControl("BTN_SEL5");
    setButtonColors(button5);
    button5.extent().getCenter(x, y);
    _classButtons.push_back({ &button5, glm::vec2(x, y), randomCharacter(Gender::Female, _gameId == GameID::KotOR ? ClassType::Scout : ClassType::JediSentinel) });

    Control &button6 = getControl("BTN_SEL6");
    setButtonColors(button6);
    button6.extent().getCenter(x, y);
    _classButtons.push_back({ &button6, glm::vec2(x, y), randomCharacter(Gender::Female, _gameId == GameID::KotOR ? ClassType::Scoundrel : ClassType::JediConsular) });

    _enlargedButtonSize = glm::vec2(button1.extent().width, button1.extent().height);
    _defaultButtonSize = glm::vec2(button2.extent().width, button2.extent().height);

    setClassButtonEnlarged(0, false);
}

void ClassSelection::setButtonColors(Control &control) {
    control.setBorderColor(getBaseColor(_gameId));
    control.setHilightColor(getHilightColor(_gameId));
}

void ClassSelection::setClassButtonEnlarged(int index, bool enlarged) {
    ClassButton &button = _classButtons[index];
    Control &control(*button.control);
    Control::Extent extent(control.extent());

    extent.width = static_cast<int>(enlarged ? _enlargedButtonSize.x : _defaultButtonSize.x);
    extent.height = static_cast<int>(enlarged ? _enlargedButtonSize.y : _defaultButtonSize.y);
    extent.left = static_cast<int>(button.center.x - 0.5f * extent.width);
    extent.top = static_cast<int>(button.center.y - 0.5f * extent.height);

    control.setExtent(move(extent));
}

void ClassSelection::configureClassModels() {
    switch (_gameId) {
        case GameID::TSL:
            configureClassModel(0, Gender::Male, ClassType::JediConsular);
            configureClassModel(1, Gender::Male, ClassType::JediSentinel);
            configureClassModel(2, Gender::Male, ClassType::JediGuardian);
            configureClassModel(3, Gender::Female, ClassType::JediGuardian);
            configureClassModel(4, Gender::Female, ClassType::JediSentinel);
            configureClassModel(5, Gender::Female, ClassType::JediConsular);
            break;
        default:
            configureClassModel(0, Gender::Male, ClassType::Scoundrel);
            configureClassModel(1, Gender::Male, ClassType::Scout);
            configureClassModel(2, Gender::Male, ClassType::Soldier);
            configureClassModel(3, Gender::Female, ClassType::Soldier);
            configureClassModel(4, Gender::Female, ClassType::Scout);
            configureClassModel(5, Gender::Female, ClassType::Scoundrel);
            break;
    }
}

void ClassSelection::configureClassModel(int index, Gender gender, ClassType clazz) {
    Control::Extent extent;
    extent.left = _classButtons[index].center.x - _defaultButtonSize.x / 2;
    extent.top = _classButtons[index].center.y - _defaultButtonSize.y / 2;
    extent.width = _defaultButtonSize.x;
    extent.height = _defaultButtonSize.y;

    float aspect = extent.width / static_cast<float>(extent.height);

    unique_ptr<Control::Scene3D> scene(SceneBuilder(_gfxOpts)
        .aspect(aspect)
        .depth(0.1f, 10.0f)
        .modelSupplier([this, &index](SceneGraph &sceneGraph) { return getCharacterModel(_classButtons[index].character, sceneGraph); })
        .modelScale(kModelScale)
        .cameraFromModelNode("camerahook")
        .ambientLightColor(glm::vec3(0.2f))
        .build());

    Control &control = getControl("3D_MODEL" + to_string(index + 1));
    control.setExtent(extent);
    control.setScene3D(move(scene));
}

shared_ptr<ModelSceneNode> ClassSelection::getCharacterModel(const std::shared_ptr<StaticCreatureBlueprint> &character, SceneGraph &sceneGraph) {
    auto root = make_shared<ModelSceneNode>(ModelSceneNode::Classification::Other, Models::instance().get("cgbody_light"), &sceneGraph);

    // Attach character model to the root model
    auto objectFactory = make_unique<ObjectFactory>(_game, &sceneGraph);
    unique_ptr<Creature> creature(objectFactory->newCreature());
    creature->load(character);
    creature->setFacing(-glm::half_pi<float>());
    creature->updateModelAnimation();
    root->attach("cgbody_light", creature->getModelSceneNode());

    return move(root);
}

void ClassSelection::onFocusChanged(const string &control, bool focus) {
    GameGUI::onFocusChanged(control, focus);

    int idx = getClassButtonIndexByTag(control);
    if (idx == -1) return;

    if (focus) {
        setClassButtonEnlarged(idx, true);
    } else {
        setClassButtonEnlarged(idx, false);
    }

    ClassButton &button = _classButtons[idx];
    ClassType clazz = button.character->attributes().getEffectiveClass();

    string classText(Resources::instance().getString(g_genderStrRefs[button.character->gender()]));
    classText += " " + Classes::instance().get(clazz)->name();

    string descText(Resources::instance().getString(g_classDescStrRefs[clazz]));

    getControl("LBL_CLASS").setTextMessage(classText);
    getControl("LBL_DESC").setTextMessage(descText);
}

int ClassSelection::getClassButtonIndexByTag(const string &tag) const {
    for (int i = 0; i < _classButtons.size(); ++i) {
        if (_classButtons[i].control->tag() == tag) {
            return i;
        }
    }

    return -1;
}

void ClassSelection::onClick(const string &control) {
    GameGUI::onClick(control);

    CharacterGeneration &charGen = _game->characterGeneration();
    int idx = getClassButtonIndexByTag(control);
    if (idx != -1) {
        charGen.setCharacter(*_classButtons[idx].character);
        charGen.openQuickOrCustom();
        return;
    }
    if (control == "BTN_BACK") {
        charGen.cancel();
    }
}

} // namespace game

} // namespace reone
