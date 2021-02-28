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

#include "abilities.h"

#include "../../../gui/control/listbox.h"
#include "../../../render/textures.h"
#include "../../../resource/resources.h"

#include "../../game.h"

#include "../colorutil.h"

using namespace std;

using namespace reone::gui;
using namespace reone::render;
using namespace reone::resource;

namespace reone {

namespace game {

static constexpr int kStrRefSkillRank = 1579;
static constexpr int kStrRefBonus = 32129;
static constexpr int kStrRefTotalRank = 41904;

AbilitiesMenu::AbilitiesMenu(Game *game) :
    GameGUI(game->gameId(), game->options().graphics),
    _game(game) {

    _resRef = getResRef("abilities");
    _backgroundType = BackgroundType::Menu;

    initForGame();
}

void AbilitiesMenu::load() {
    GUI::load();

    hideControl("BTN_CHARLEFT");
    hideControl("BTN_CHARRIGHT");

    disableControl("BTN_SKILLS");
    disableControl("BTN_POWERS");
    disableControl("BTN_FEATS");

    setControlText("LBL_SKILLRANK", Resources::instance().getString(kStrRefSkillRank));
    setControlText("LBL_BONUS", Resources::instance().getString(kStrRefBonus));
    setControlText("LBL_TOTAL", Resources::instance().getString(kStrRefTotalRank));

    setControlText("LBL_RANKVAL", "");
    setControlText("LBL_BONUSVAL", "");
    setControlText("LBL_TOTALVAL", "");

    setControlText("LBL_NAME", "");

    auto &lbDesc = getControl<ListBox>("LB_DESC");
    lbDesc.setProtoMatchContent(true);

    loadSkills();
}

static shared_ptr<Texture> getFrameTexture(GameID gameId) {
    string resRef;
    if (gameId == GameID::TSL) {
        resRef = "uibit_eqp_itm1";
    } else {
        resRef = "lbl_hex_3";
    }
    return Textures::instance().get(resRef, TextureUsage::GUI);
}

void AbilitiesMenu::loadSkills() {
    shared_ptr<TwoDA> skills(Resources::instance().get2DA("skills"));
    for (int row = 0; row < skills->getRowCount(); ++row) {
        auto skill = static_cast<Skill>(row);

        SkillInfo skillInfo;
        skillInfo.skill = skill;
        skillInfo.name = Resources::instance().getString(skills->getInt(row, "name"));
        skillInfo.description = Resources::instance().getString(skills->getInt(row, "description"));
        skillInfo.icon = Textures::instance().get(skills->getString(row, "icon"), TextureUsage::GUI);

        _skills.insert(make_pair(skill, move(skillInfo)));
    }

    ListBox &lbAbility = getControl<ListBox>("LB_ABILITY");
    lbAbility.clearItems();
    for (auto &skill : _skills) {
        ListBox::Item item;
        item.tag = to_string(static_cast<int>(skill.second.skill));
        item.text = skill.second.name;
        item.iconFrame = getFrameTexture(_gameId);
        item.iconTexture = skill.second.icon;
        lbAbility.addItem(move(item));
    }
}

void AbilitiesMenu::refreshControls() {
    refreshPortraits();
}

void AbilitiesMenu::refreshPortraits() {
    if (_gameId != GameID::KotOR) return;

    Party &party = _game->party();
    shared_ptr<Creature> partyLeader(party.getLeader());
    shared_ptr<Creature> partyMember1(party.getMember(1));
    shared_ptr<Creature> partyMember2(party.getMember(2));

    Control &btnChange1 = getControl("BTN_CHANGE1");
    btnChange1.setBorderFill(partyMember1 ? partyMember1->portrait() : nullptr);
    btnChange1.setHilightFill(partyMember1 ? partyMember1->portrait() : nullptr);

    Control &btnChange2 = getControl("BTN_CHANGE2");
    btnChange2.setBorderFill(partyMember2 ? partyMember2->portrait() : nullptr);
    btnChange2.setHilightFill(partyMember2 ? partyMember2->portrait() : nullptr);
}

void AbilitiesMenu::onClick(const string &control) {
    GameGUI::onClick(control);

    if (control == "BTN_EXIT") {
        _game->openInGame();
    }
}

void AbilitiesMenu::onListBoxItemClick(const string &control, const string &item) {
    if (control == "LB_ABILITY") {
        auto skill = static_cast<Skill>(stoi(item));
        auto maybeSkillInfo = _skills.find(skill);
        if (maybeSkillInfo != _skills.end()) {
            shared_ptr<Creature> partyLeader(_game->party().getLeader());
            setControlText("LBL_RANKVAL", to_string(partyLeader->attributes().skills().getRank(skill)));
            setControlText("LBL_BONUSVAL", "0");
            setControlText("LBL_TOTALVAL", to_string(partyLeader->attributes().skills().getRank(skill)));

            auto &lbName = getControl("LBL_NAME");
            lbName.setTextMessage(maybeSkillInfo->second.name);

            auto &lbDesc = getControl<ListBox>("LB_DESC");
            lbDesc.clearItems();
            lbDesc.addTextLinesAsItems(maybeSkillInfo->second.description);
        }
    }
}

} // namespace game

} // namespace reone
