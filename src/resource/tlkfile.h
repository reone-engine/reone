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

#include "binfile.h"

namespace reone {

namespace resource {

struct TalkTableString {
    std::string text;
    std::string soundResRef;
};

class TlkFile;

struct TalkTable {
public:
    TalkTable() = default;

    const TalkTableString &getString(int32_t ref) const;
    int stringCount() const;

private:
    std::vector<TalkTableString> _strings;

    TalkTable(const TalkTable &) = delete;
    TalkTable &operator=(const TalkTable &) = delete;

    friend class TlkFile;
};

class TlkFile : public BinaryFile {
public:
    TlkFile();
    std::shared_ptr<TalkTable> table() const;

private:
    uint32_t _stringCount { 0 };
    uint32_t _stringsOffset { 0 };
    std::shared_ptr<TalkTable> _table;

    void doLoad() override;
    void loadStrings();
};

} // namespace resource

} // namespace reone
