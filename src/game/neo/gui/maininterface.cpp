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

#include "maininterface.h"

using namespace reone::gui::neo;

namespace reone {

namespace game {

namespace neo {

void MainInterfaceGui::init() {
    load("mipc28x6");
    bindControls();

    _btnActionDown0->setFlipVertical(true);
    _btnActionDown1->setFlipVertical(true);
    _btnActionDown2->setFlipVertical(true);
    _btnActionDown3->setFlipVertical(true);

    // Pop-ups
    disableControl("LBL_ACTIONDESC");
    disableControl("LBL_ACTIONDESCBG");

    // Party
    disableControl("LBL_CMBTEFCTINC1");
    disableControl("LBL_CMBTEFCTINC2");
    disableControl("LBL_CMBTEFCTINC3");
    disableControl("LBL_CMBTEFCTRED1");
    disableControl("LBL_CMBTEFCTRED2");
    disableControl("LBL_CMBTEFCTRED3");
    disableControl("LBL_DEBILATATED1");
    disableControl("LBL_DEBILATATED2");
    disableControl("LBL_DEBILATATED3");
    disableControl("LBL_DISABLE1");
    disableControl("LBL_DISABLE2");
    disableControl("LBL_DISABLE3");
    disableControl("LBL_LEVELUP1");
    disableControl("LBL_LEVELUP2");
    disableControl("LBL_LEVELUP3");

    // Notifications
    disableControl("LBL_CASH");
    disableControl("LBL_DARKSHIFT");
    disableControl("LBL_ITEMLOST");
    disableControl("LBL_ITEMRCVD");
    disableControl("LBL_JOURNAL");
    disableControl("LBL_LIGHTSHIFT");
    disableControl("LBL_PLOTXP");
    disableControl("LBL_STEALTHXP");

    // Combat
    disableControl("BTN_CLEARALL");
    disableControl("BTN_TARGET0");
    disableControl("BTN_TARGET1");
    disableControl("BTN_TARGET2");
    disableControl("BTN_TARGETDOWN0");
    disableControl("BTN_TARGETDOWN1");
    disableControl("BTN_TARGETDOWN2");
    disableControl("BTN_TARGETUP0");
    disableControl("BTN_TARGETUP1");
    disableControl("BTN_TARGETUP2");
    disableControl("LBL_CMBTMODEMSG");
    disableControl("LBL_CMBTMSGBG");
    disableControl("LBL_COMBATBG1");
    disableControl("LBL_COMBATBG2");
    disableControl("LBL_COMBATBG3");
    disableControl("LBL_HEALTHBG");
    disableControl("LBL_NAME");
    disableControl("LBL_NAMEBG");
    disableControl("LBL_TARGET0");
    disableControl("LBL_TARGET1");
    disableControl("LBL_TARGET2");
}

void MainInterfaceGui::bindControls() {
    _btnActionDown0 = findControlByTag<Button>("BTN_ACTIONDOWN0");
    _btnActionDown1 = findControlByTag<Button>("BTN_ACTIONDOWN1");
    _btnActionDown2 = findControlByTag<Button>("BTN_ACTIONDOWN2");
    _btnActionDown3 = findControlByTag<Button>("BTN_ACTIONDOWN3");
}

} // namespace neo

} // namespace game

} // namespace reone
