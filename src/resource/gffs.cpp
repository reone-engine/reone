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

#include "gffs.h"

#include "../common/streamutil.h"

#include "format/gffreader.h"
#include "resources.h"

using namespace std;

namespace reone {

namespace resource {

shared_ptr<Gff> Gffs::get(const string &resRef, ResourceType type) {
    ResourceId id(resRef, type);
    auto maybeGff = _cache.find(id);
    if (maybeGff != _cache.end()) {
        return maybeGff->second;
    }
    shared_ptr<Gff> gff;
    auto maybeRaw = _resources.get(resRef, type);
    if (maybeRaw) {
        GffReader reader;
        reader.load(wrap(*maybeRaw));
        gff = reader.root();
    }
    auto inserted = _cache.insert(make_pair(id, gff));
    return inserted.first->second;
}

} // namespace resource

} // namespace reone
