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

#include "barkbubble.h"

#include "../game.h"

using namespace std;

using namespace reone::gui;

namespace reone {

namespace game {

BarkBubble::BarkBubble(Game *game) :
    GameGUI(game) {
    _resRef = getResRef("barkbubble");
    _scaling = ScalingMode::PositionRelativeToCenter;
}

void BarkBubble::load() {
    GUI::load();
    bindControls();

    _rootControl->setVisible(false);
    _binding.lblBarkText->setVisible(false);
}

void BarkBubble::bindControls() {
    _binding.lblBarkText = getControl<Label>("LBL_BARKTEXT");
}

void BarkBubble::update(float dt) {
    if (_timer.advance(dt)) {
        setBarkText("", 0.0f);
    }
}

void BarkBubble::setBarkText(const string &text, float duration) {
    if (text.empty()) {
        _rootControl->setVisible(false);
        _binding.lblBarkText->setVisible(false);
    } else {
        float textWidth = _binding.lblBarkText->text().font->measure(text);
        int lineCount = static_cast<int>(textWidth / static_cast<float>(_binding.lblBarkText->extent().width)) + 1;
        int padding = _binding.lblBarkText->extent().left;
        float rootHeight = lineCount * _binding.lblBarkText->text().font->height() + 2 * padding;
        float labelHeight = lineCount * _binding.lblBarkText->text().font->height();

        _rootControl->setVisible(true);
        _rootControl->setExtentHeight(static_cast<int>(rootHeight));

        _binding.lblBarkText->setExtentHeight(static_cast<int>(labelHeight));
        _binding.lblBarkText->setTextMessage(text);
        _binding.lblBarkText->setVisible(true);
    }

    if (duration > 0.0f) {
        _timer.setTimeout(duration);
    }
}

} // namespace game

} // namespace reone
