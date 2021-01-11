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

#include "quickorcustom.h"

#include "../../../gui/control/listbox.h"
#include "../../../resource/resources.h"

#include "../colorutil.h"

#include "chargen.h"

using namespace std;

using namespace reone::gui;
using namespace reone::render;
using namespace reone::resource;

namespace reone {

namespace game {

constexpr int kStrRefQuickHelpText = 241;
constexpr int kStrRefCustomHelpText = 242;

QuickOrCustom::QuickOrCustom(CharacterGeneration *charGen, GameVersion version, const GraphicsOptions &opts) :
    GameGUI(version, opts),
    _charGen(charGen) {

    _resRef = getResRef("qorcpnl");

    initForGame();
}

void QuickOrCustom::load() {
    GUI::load();

    if (_version == GameVersion::KotOR) {
        setControlDiscardColor("LBL_RBG", glm::vec3(0.0f, 0.0f, 0.082353f));
    }
}

void QuickOrCustom::onClick(const string &control) {
    if (control == "QUICK_CHAR_BTN") {
        _charGen->startQuick();
    } else if (control == "CUST_CHAR_BTN") {
        _charGen->startCustom();
    } else if (control == "BTN_BACK") {
        _charGen->openClassSelection();
    }
}

void QuickOrCustom::onFocusChanged(const string &control, bool focus) {
    if (focus) {
        string text;
        if (control == "QUICK_CHAR_BTN") {
            text = Resources::instance().getString(kStrRefQuickHelpText);
        } else if (control == "CUST_CHAR_BTN") {
            text = Resources::instance().getString(kStrRefCustomHelpText);
        }
        ListBox::Item item;
        item.text = text;

        ListBox &lbDesc = getControl<ListBox>("LB_DESC");
        lbDesc.clearItems();
        lbDesc.addItem(move(item));
    }
}

} // namespace game

} // namespace reone
