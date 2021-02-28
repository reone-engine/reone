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

#include "curfile.h"

#include "../textureutil.h"

using namespace std;

namespace reone {

namespace render {

CurFile::CurFile() : BinaryFile(0) {
}

void CurFile::doLoad() {
    loadHeader();
    loadData();
}

void CurFile::loadHeader() {
    ignore(4);

    uint32_t size = readUint32();

    _width = readInt32();
    _height = readInt32();

    uint16_t planes = readUint16();

    _bitCount = readUint16();

    uint32_t compression = readUint32();
}

void CurFile::loadData() {
    seek(44);

    int pixelCount = _width * _width;
    int colorCount = _bitCount == 8 ? 256 : 16;

    ByteArray palette(_reader->getArray<char>(4 * colorCount));
    ByteArray xorData(_reader->getArray<char>(pixelCount));
    ByteArray andData(_reader->getArray<char>(pixelCount / 8));

    auto pixels = make_shared<ByteArray>(4 * pixelCount);

    for (int y = 0; y < _width; ++y) {
        for (int x = 0; x < _width; ++x) {
            int pixelIdx = (y * _width) + x;
            int offMipMap = 4 * pixelIdx;
            int offPalette = 4 * static_cast<uint8_t>(xorData[pixelIdx]);

            *(pixels->data() + offMipMap + 0) = palette[offPalette + 0];
            *(pixels->data() + offMipMap + 1) = palette[offPalette + 1];
            *(pixels->data() + offMipMap + 2) = palette[offPalette + 2];
            *(pixels->data() + offMipMap + 3) = (andData[pixelIdx / 8] & (1 << (7 - x % 8))) ? 0 : 0xff;
        }
    }

    Texture::MipMap mipMap;
    mipMap.width = _width;
    mipMap.height = _width;
    mipMap.pixels = move(pixels);

    Texture::Layer layer;
    layer.mipMaps.push_back(move(mipMap));

    _texture = make_shared<Texture>("", getTextureProperties(TextureUsage::GUI));
    _texture->init();
    _texture->bind();
    _texture->setPixels(_width, _width, PixelFormat::BGRA, vector<Texture::Layer> { layer });
}

} // namespace render

} // namespace reone
