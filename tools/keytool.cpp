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

#include "tools.h"

#include <boost/format.hpp>

#include "../src/common/log.h"

using namespace reone::resource;

namespace fs = boost::filesystem;

namespace reone {

namespace tools {

void KeyTool::list(const fs::path &path, const fs::path &keyPath) const {
    KeyFile key;
    key.load(path);

    for (auto &file : key.files()) {
        info(file.filename);
    }
}

} // namespace tools

} // namespace reone
