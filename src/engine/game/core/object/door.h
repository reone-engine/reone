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

#include "../../../resource/format/gffreader.h"
#include "../../../scene/node/walkmesh.h"

namespace reone {

namespace game {

class Door : public SpatialObject {
public:
    Door(
        uint32_t id,
        std::string sceneName,
        Game &game,
        Services &services) :
        SpatialObject(
            id,
            ObjectType::Door,
            std::move(sceneName),
            game,
            services) {
    }

    void loadFromGIT(const resource::GffStruct &gffs);
    void loadFromBlueprint(const std::string &resRef);

    bool isSelectable() const override;

    void open(const std::shared_ptr<Object> &triggerrer);
    void close(const std::shared_ptr<Object> &triggerrer);

    bool isLocked() const { return _locked; }
    bool isStatic() const { return _static; }
    bool isKeyRequired() const { return _keyRequired; }

    const std::string &getOnOpen() const { return _onOpen; }
    const std::string &getOnFailToOpen() const { return _onFailToOpen; }

    int genericType() const { return _genericType; }
    const std::string &linkedToModule() const { return _linkedToModule; }
    const std::string &linkedTo() const { return _linkedTo; }
    const std::string &transitionDestin() const { return _transitionDestin; }

    void setLocked(bool locked);

    // Walkmeshes

    std::shared_ptr<scene::WalkmeshSceneNode> walkmeshOpen1() const { return _walkmeshOpen1; }
    std::shared_ptr<scene::WalkmeshSceneNode> walkmeshOpen2() const { return _walkmeshOpen2; }
    std::shared_ptr<scene::WalkmeshSceneNode> walkmeshClosed() const { return _walkmeshClosed; }

    // END Walkmeshes

private:
    bool _locked {false};
    int _genericType {0};
    bool _static {false};
    bool _keyRequired {false};
    std::string _linkedToModule;
    std::string _linkedTo;
    int _linkedToFlags {0};
    std::string _transitionDestin;
    Faction _faction {Faction::Invalid};
    int _openLockDC {0};
    int _hardness {0};
    int _fortitude {0};
    bool _lockable {false};
    std::string _keyName;

    // Walkmeshes

    std::shared_ptr<scene::WalkmeshSceneNode> _walkmeshOpen1;
    std::shared_ptr<scene::WalkmeshSceneNode> _walkmeshOpen2;
    std::shared_ptr<scene::WalkmeshSceneNode> _walkmeshClosed;

    // END Walkmeshes

    // Scripts

    std::string _onOpen;
    std::string _onFailToOpen;
    std::string _onClick;
    std::string _onClosed;
    std::string _onDamaged;
    std::string _onLock;
    std::string _onUnlock;
    std::string _onMeleeAttacked;
    std::string _onSpellCastAt;

    // END Scripts

    void loadUTD(const resource::GffStruct &utd);
    void loadTransformFromGIT(const resource::GffStruct &gffs);

    void updateTransform() override;
};

} // namespace game

} // namespace reone
