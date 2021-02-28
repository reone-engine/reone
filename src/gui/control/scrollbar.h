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

#include "control.h"

namespace reone {

namespace gui {

class ScrollBar : public Control {
public:
    struct ScrollState {
        int count { 0 }; /**< total number of list items */
        int numVisible { 0 }; /**< number of visible list items */
        int offset { 0 }; /**< offset into the list of items  */
    };

    ScrollBar(GUI *gui);

    void load(const resource::GffStruct &gffs) override;
    void render(const glm::ivec2 &offset, const std::vector<std::string> &text) override;

    void setScrollState(ScrollState state);

private:
    struct Direction {
        std::shared_ptr<render::Texture> image;
    };

    struct Thumb {
        std::shared_ptr<render::Texture> image;
    };

    Direction _dir;
    Thumb _thumb;
    ScrollState _state;

    void drawThumb(const glm::ivec2 &offset);
    void drawArrows(const glm::ivec2 &offset);

    void drawUpArrow(const glm::ivec2 &offset);
    void drawDownArrow(const glm::ivec2 &offset);
};

} // namespace gui

} // namespace reone
