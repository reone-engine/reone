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

#include "../../resource/format/gffreader.h"

#include "../object.h"

namespace reone {

namespace game {

class Waypoint : public Object {
public:
    Waypoint(
        uint32_t id,
        std::string sceneName,
        Game &game,
        ServicesView &services) :
        Object(
            id,
            ObjectType::Waypoint,
            std::move(sceneName),
            game,
            services) {
    }

    void loadFromGIT(const resource::Gff &gffs);
    void loadFromBlueprint(const std::string &resRef);

    bool isMapNoteEnabled() const { return _mapNoteEnabled; }

    const std::string &mapNote() const { return _mapNote; }

private:
    int _appearance {0};
    bool _hasMapNote {false};
    std::string _mapNote;
    bool _mapNoteEnabled {false};

    void loadTransformFromGIT(const resource::Gff &gffs);

    void loadUTW(const resource::Gff &utw);
};

} // namespace game

} // namespace reone
