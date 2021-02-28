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

#include <string>

#include "SDL2/SDL_events.h"

namespace reone {

namespace gui {

struct TextInputFlags {
    static constexpr int digits = 1;
    static constexpr int letters = 2;
    static constexpr int whitespace = 4;
    static constexpr int punctuation = 8;

    static constexpr int lettersWhitespace = letters | whitespace;
    static constexpr int console = digits | letters | whitespace | punctuation;
};

class TextInput {
public:
    TextInput(int mask);

    void clear();
    bool handle(const SDL_Event &event);

    const std::string &text() const { return _text; }

    void setText(std::string text);

private:
    int _mask { 0 };
    std::string _text;

    bool handleKeyDown(const SDL_KeyboardEvent &event);
    bool isKeyAllowed(const SDL_Keysym &key) const;
};

} // namespace gui

} // namespace reone
