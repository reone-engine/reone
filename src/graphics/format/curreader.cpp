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

#include "curreader.h"

#include "../texture.h"
#include "../textureutil.h"

using namespace std;

namespace reone {

namespace graphics {

CurReader::CurReader() :
    BinaryReader(0) {
}

void CurReader::doLoad() {
    loadHeader();
    loadData();
}

void CurReader::loadHeader() {
    ignore(4);

    uint32_t size = readUint32();

    _width = readInt32();
    _height = readInt32();

    uint16_t planes = readUint16();

    _bitCount = readUint16();

    uint32_t compression = readUint32();
}

void CurReader::loadData() {
    seek(44);

    int numPixels = _width * _width;
    int colorCount = _bitCount == 8 ? 256 : 16;

    ByteArray palette(_reader->getBytes(4 * colorCount));
    ByteArray xorData(_reader->getBytes(numPixels));
    ByteArray andData(_reader->getBytes(numPixels / 8));

    auto pixels = make_shared<ByteArray>(4 * numPixels);

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

    _texture = make_shared<Texture>("", getTextureProperties(TextureUsage::GUI));
    _texture->setPixels(_width, _width, PixelFormat::BGRA8, Texture::Layer {move(pixels)});
}

} // namespace graphics

} // namespace reone
