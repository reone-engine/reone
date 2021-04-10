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

#include <cstdint>
#include <memory>

#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>

#include "../talktable.h"

namespace reone {

namespace resource {

class TlkWriter : boost::noncopyable {
public:
    TlkWriter(std::shared_ptr<TalkTable> talkTable);

    void save(const boost::filesystem::path &path);

private:
    struct FileHeader {
        uint32_t languageId { 0 };
        uint32_t numStrings { 0 };
        uint32_t offStringEntries { 0 };
    };

    struct StringDataElement {
        uint32_t flags { 0 };
        char soundResRef[16];
        uint32_t volumeVariance { 0 };
        uint32_t pitchVariance { 0 };
        uint32_t offString { 0 };
        uint32_t stringSize { 0 };
        float soundLength { 0.0f };
    };

    std::shared_ptr<TalkTable> _talkTable;
};

} // namespace resource

} // namespace reone