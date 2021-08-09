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

#include "feats.h"

#include "../../../gui/control/button.h"

#include "chargen.h"

using namespace std;

using namespace reone::gui;
using namespace reone::graphics;
using namespace reone::resource;

namespace reone {

namespace game {

CharGenFeats::CharGenFeats(CharacterGeneration *charGen, Game *game) :
    GameGUI(game),
    _charGen(charGen) {

    _resRef = getResRef("ftchrgen");

    initForGame();
}

void CharGenFeats::load() {
    GUI::load();
    bindControls();

    _binding.btnSelect->setDisabled(true);
    _binding.btnRecommended->setDisabled(true);
}

void CharGenFeats::bindControls() {
    _binding.btnSelect = getControl<Button>("BTN_SELECT");
    _binding.btnRecommended = getControl<Button>("BTN_RECOMMENDED");
}

void CharGenFeats::onClick(const string &control) {
    GameGUI::onClick(control);

    if (control == "BTN_ACCEPT") {
        _charGen->goToNextStep();
        _charGen->openSteps();
    } else if (control == "BTN_BACK") {
        _charGen->openSteps();
    }
}

} // namespace game

} // namespace reone
