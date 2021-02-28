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

#include "2dafile.h"

using namespace std;

namespace fs = boost::filesystem;

namespace reone {

namespace resource {

static constexpr int kSignatureSize = 8;
static const char kSignature[] = "2DA V2.b";

TwoDaFile::TwoDaFile() : BinaryFile(kSignatureSize, kSignature), _twoDa(make_shared<TwoDA>()) {
}

void TwoDaFile::doLoad() {
    ignore(1); // newline
    loadHeaders();

    _rowCount = readUint32();

    loadLabels();
    loadRows();
}

void TwoDaFile::loadHeaders() {
    string token;
    while (readToken(token)) {
        _twoDa->_columns.push_back(token);
    }
}

bool TwoDaFile::readToken(string &token) {
    size_t pos = tell();

    char buf[256];
    streamsize chRead = _in->rdbuf()->sgetn(buf, sizeof(buf));
    const char *pch = buf;

    for (; pch - buf < chRead; ++pch) {
        if (*pch == '\0') {
            seek(pos + pch - buf + 1);
            return false;
        }
        if (*pch == '\t') {
            string s(buf, pch - buf);
            seek(pos + pch - buf + 1);
            token = move(s);
            return true;
        }
    }

    throw runtime_error("2DA token not terminated");
}

void TwoDaFile::loadLabels() {
    string token;
    for (int i = 0; i < _rowCount; ++i) {
        readToken(token);
    }
}

void TwoDaFile::loadRows() {
    _twoDa->_rows.reserve(_rowCount);

    int columnCount = static_cast<int>(_twoDa->_columns.size());
    int cellCount = _rowCount * columnCount;
    vector<uint16_t> offsets(cellCount);
    for (int i = 0; i < cellCount; ++i) {
        offsets[i] = readUint16();
    }

    uint16_t dataSize = readUint16();
    size_t pos = tell();

    for (int i = 0; i < _rowCount; ++i) {
        TwoDA::Row row;
        for (int j = 0; j < columnCount; ++j) {
            int cellIdx = i * columnCount + j;
            size_t off = pos + offsets[cellIdx];
            row.values.push_back(readCStringAt(off));
        }
        _twoDa->_rows.push_back(row);
    }
}

} // namespace bioware

} // namespace reone
