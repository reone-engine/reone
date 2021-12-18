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

#include "cursors.h"

#include "../../common/streamutil.h"
#include "../../graphics/cursor.h"
#include "../../graphics/format/curreader.h"
#include "../../graphics/texture.h"
#include "../../resource/resources.h"

using namespace std;

using namespace reone::game;
using namespace reone::graphics;
using namespace reone::resource;

namespace reone {

namespace kotor {

static unordered_map<CursorType, pair<uint32_t, uint32_t>> g_groupNamesByType {
    {CursorType::Default, {1, 2}},
    {CursorType::Talk, {11, 12}},
    {CursorType::Door, {23, 24}},
    {CursorType::Pickup, {25, 26}},
    {CursorType::DisableMine, {33, 34}},
    {CursorType::RecoverMine, {37, 38}},
    {CursorType::Attack, {51, 52}}};

void Cursors::deinit() {
    _cache.clear();
}

shared_ptr<Cursor> Cursors::get(CursorType type) {
    auto maybeCursor = _cache.find(type);
    if (maybeCursor != _cache.end()) {
        return maybeCursor->second;
    }
    const pair<uint32_t, uint32_t> &groupNames = getCursorGroupNames(type);
    vector<uint32_t> cursorNamesUp(getCursorNamesFromCursorGroup(groupNames.first));
    if (cursorNamesUp.empty()) {
        return nullptr;
    }
    vector<uint32_t> cursorNamesDown(getCursorNamesFromCursorGroup(groupNames.second));
    if (cursorNamesDown.empty()) {
        return nullptr;
    }
    shared_ptr<Texture> textureUp(newTextureFromCursor(cursorNamesUp.back()));
    textureUp->init();
    shared_ptr<Texture> textureDown(newTextureFromCursor(cursorNamesDown.back()));
    textureDown->init();

    auto cursor = make_shared<Cursor>(textureUp, textureDown, _graphicsContext, _meshes, _shaders, _window);
    _cache.insert(make_pair(type, cursor));

    return move(cursor);
}

const pair<uint32_t, uint32_t> &Cursors::getCursorGroupNames(CursorType type) {
    auto maybeGroupNames = g_groupNamesByType.find(type);
    if (maybeGroupNames == g_groupNamesByType.end()) {
        throw logic_error("Cursor groups not found by type " + to_string(static_cast<int>(type)));
    }
    return maybeGroupNames->second;
}

vector<uint32_t> Cursors::getCursorNamesFromCursorGroup(uint32_t name) {
    shared_ptr<ByteArray> bytes(_resources.getFromExe(name, PEResourceType::CursorGroup));
    if (!bytes) {
        return vector<uint32_t>();
    }

    StreamReader reader(wrap(bytes));
    reader.ignore(4); // Reserved, ResType
    uint16_t resCount = reader.getUint16();

    vector<uint32_t> cursorNames;
    for (uint16_t i = 0; i < resCount; ++i) {
        reader.ignore(12); // Cursor, Planes, BitCount, BytesInRes
        uint16_t cursorId = reader.getUint16();
        cursorNames.push_back(static_cast<uint32_t>(cursorId));
    }

    return move(cursorNames);
}

shared_ptr<Texture> Cursors::newTextureFromCursor(uint32_t name) {
    shared_ptr<ByteArray> bytes(_resources.getFromExe(name, PEResourceType::Cursor));

    CurReader cur;
    cur.load(wrap(bytes));

    return cur.texture();
}

} // namespace kotor

} // namespace reone
