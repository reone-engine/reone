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

#include "lip.h"

#include "../../graphics/format/lipreader.h"
#include "../../graphics/format/lipwriter.h"

#include "tinyxml2.h"

using namespace std;

using namespace tinyxml2;

using namespace reone::graphics;

namespace fs = boost::filesystem;

namespace reone {

void LipTool::invoke(Operation operation, const fs::path &target, const fs::path &gamePath, const fs::path &destPath) {
    if (operation == Operation::ToXML) {
        toXML(target, destPath);
    } else if (operation == Operation::ToLIP) {
        toLIP(target, destPath);
    }
}

void LipTool::toXML(const fs::path &path, const fs::path &destPath) {
    auto reader = LipReader("");
    reader.load(path);

    auto animation = reader.animation();

    auto xmlPath = destPath;
    xmlPath.append(path.filename().string() + ".xml");
    auto fp = fopen(xmlPath.string().c_str(), "wb");

    auto printer = XMLPrinter(fp);
    printer.PushHeader(false, true);
    printer.OpenElement("animation");
    printer.PushAttribute("length", animation->length());
    for (auto &keyframe : animation->keyframes()) {
        printer.OpenElement("keyframe");
        printer.PushAttribute("time", keyframe.time);
        printer.PushAttribute("shape", keyframe.shape);
        printer.CloseElement();
    }
    printer.CloseElement();

    fclose(fp);
}

void LipTool::toLIP(const fs::path &path, const fs::path &destPath) {
    auto fp = fopen(path.string().c_str(), "rb");

    auto document = XMLDocument();
    document.LoadFile(fp);

    auto rootElement = document.RootElement();
    if (!rootElement) {
        cerr << "XML is empty" << endl;
        fclose(fp);
        return;
    }

    auto length = rootElement->FloatAttribute("length");
    auto keyframes = vector<LipAnimation::Keyframe>();
    for (auto element = rootElement->FirstChildElement(); element; element = element->NextSiblingElement()) {
        keyframes.push_back(LipAnimation::Keyframe {
            element->FloatAttribute("time"),
            static_cast<uint8_t>(element->UnsignedAttribute("shape"))});
    }
    auto animation = LipAnimation("", length, move(keyframes));

    vector<string> tokens;
    boost::split(
        tokens,
        path.filename().string(),
        boost::is_any_of("."),
        boost::token_compress_on);

    auto lipPath = fs::path(destPath);
    lipPath.append(tokens[0] + ".lip");

    auto writer = LipWriter(move(animation));
    writer.save(lipPath);
}

bool LipTool::supports(Operation operation, const fs::path &target) const {
    return !fs::is_directory(target) &&
           ((target.extension() == ".lip" && operation == Operation::ToXML) ||
            (target.extension() == ".xml" && operation == Operation::ToLIP));
}

} // namespace reone
