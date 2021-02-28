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

#include <algorithm>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "../src/common/pathutil.h"
#include "../src/resource/typeutil.h"

using namespace std;

using namespace reone::resource;

namespace fs = boost::filesystem;

namespace reone {

namespace tools {

void KeyBifTool::invoke(Operation operation, const fs::path &target, const fs::path &gamePath, const fs::path &destPath) {
    bool isKey = target.extension() == ".key";
    if (isKey) {
        KeyFile key;
        key.load(target);

        listKEY(key);

    } else {
        fs::path keyPath(getPathIgnoreCase(gamePath, "chitin.key"));

        KeyFile key;
        key.load(keyPath);

        int bifIdx = -1;
        for (size_t i = 0; i < key.files().size(); ++i) {
            if (boost::ends_with(key.getFilename(static_cast<int>(i)), target.filename().string())) {
                bifIdx = static_cast<int>(i);
                break;
            }
        }
        if (bifIdx == -1) return;

        BifFile bif;
        bif.load(target);

        if (operation == Operation::List) {
            listBIF(key, bif, bifIdx);
        } else if (operation == Operation::Extract) {
            extractBIF(key, bif, bifIdx, destPath);
        }
    }
}

void KeyBifTool::listKEY(const KeyFile &key) {
    for (auto &file : key.files()) {
        cout << file.filename << " " << file.fileSize << endl;
    }
}

void KeyBifTool::listBIF(const KeyFile &key, const BifFile &bif, int bifIdx) {
    for (auto &keyEntry : key.keys()) {
        if (keyEntry.bifIdx == bifIdx) {
            cout << keyEntry.resRef << " " << getExtByResType(keyEntry.resType) << endl;
        }
    }
}

void KeyBifTool::extractBIF(const KeyFile &key, BifFile &bif, int bifIdx, const fs::path &destPath) {
    if (!fs::is_directory(destPath) || !fs::exists(destPath)) return;

    for (auto &keyEntry : key.keys()) {
        if (keyEntry.bifIdx != bifIdx) continue;

        string ext(getExtByResType(keyEntry.resType));
        cout << "Extracting " + keyEntry.resRef << " " << ext << endl;
        unique_ptr<ByteArray> data(bif.getResourceData(keyEntry.resIdx));

        fs::path resPath(destPath);
        resPath.append(keyEntry.resRef + "." + ext);

        fs::ofstream out(resPath, ios::binary);
        out.write(&(*data)[0], data->size()); 
    }
}

bool KeyBifTool::supports(Operation operation, const fs::path &target) const {
    if (fs::is_directory(target)) return false;

    bool key = target.extension() == ".key";
    bool bif = target.extension() == ".bif";

    if (key) {
        return operation == Operation::List;
    } else if (bif) {
        return operation == Operation::List || operation == Operation::Extract;
    } else {
        return false;
    }
}

} // namespace tools

} // namespace reone
