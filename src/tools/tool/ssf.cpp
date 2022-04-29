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

#include "ssf.h"

#include "../../game/format/ssfreader.h"
#include "../../game/format/ssfwriter.h"

#include "tinyxml2.h"

using namespace std;

using namespace tinyxml2;

using namespace reone::game;

namespace fs = boost::filesystem;

namespace reone {

void SsfTool::invoke(Operation operation, const fs::path &target, const fs::path &gamePath, const fs::path &destPath) {
    if (operation == Operation::ToXML) {
        toXML(target, destPath);
    } else if (operation == Operation::ToSSF) {
        toSSF(target, destPath);
    }
}

void SsfTool::toXML(const fs::path &path, const fs::path &destPath) {
    auto reader = SsfReader();
    reader.load(path);

    auto soundSet = reader.soundSet();

    auto xmlPath = destPath;
    xmlPath.append(path.filename().string() + ".xml");
    auto fp = fopen(xmlPath.string().c_str(), "wb");

    auto printer = XMLPrinter(fp);
    printer.PushHeader(false, true);
    printer.OpenElement("soundset");
    for (size_t i = 0; i < soundSet.size(); ++i) {
        printer.OpenElement("sound");
        printer.PushAttribute("index", i);
        printer.PushAttribute("strref", soundSet[i]);
        printer.CloseElement();
    }
    printer.CloseElement();

    fclose(fp);
}

void SsfTool::toSSF(const fs::path &path, const fs::path &destPath) {
    auto fp = fopen(path.string().c_str(), "rb");

    auto document = XMLDocument();
    document.LoadFile(fp);

    auto rootElement = document.RootElement();
    if (!rootElement) {
        cerr << "XML is empty" << endl;
        fclose(fp);
        return;
    }

    auto soundSet = vector<uint32_t>();
    for (auto element = rootElement->FirstChildElement(); element; element = element->NextSiblingElement()) {
        auto strref = element->UnsignedAttribute("strref");
        soundSet.push_back(strref);
    }

    vector<string> tokens;
    boost::split(
        tokens,
        path.filename().string(),
        boost::is_any_of("."),
        boost::token_compress_on);

    auto ssfPath = fs::path(destPath);
    ssfPath.append(tokens[0] + ".ssf");

    auto writer = SsfWriter(move(soundSet));
    writer.save(ssfPath);
}

bool SsfTool::supports(Operation operation, const fs::path &target) const {
    return (operation == Operation::ToXML && target.extension() == ".ssf") ||
           (operation == Operation::ToSSF && target.extension() == ".xml");
}

} // namespace reone
