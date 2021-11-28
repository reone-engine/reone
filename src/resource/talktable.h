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

namespace reone {

namespace resource {

struct TalkTableString {
    std::string text;
    std::string soundResRef;
};

class TalkTable : boost::noncopyable {
public:
    void addString(TalkTableString &&string);

    int getStringCount() const;
    const TalkTableString &getString(int index) const;

private:
    std::vector<TalkTableString> _strings;
};

} // namespace resource

} // namespace reone
