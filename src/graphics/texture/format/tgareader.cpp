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

#include "tgareader.h"

#include "../../../common/logutil.h"

#include "../texture.h"
#include "../textureutil.h"

using namespace std;

namespace reone {

namespace graphics {

TgaReader::TgaReader(const string &resRef, TextureUsage usage) :
    BinaryReader(0), _resRef(resRef), _usage(usage) {
}

void TgaReader::doLoad() {
    uint8_t idLength = readByte();

    ignore(1);

    _dataType = static_cast<TGADataType>(readByte());
    switch (_dataType) {
    case TGADataType::Grayscale:
    case TGADataType::RGBA:
    case TGADataType::RGBA_RLE:
        break;
    default:
        warn("TGA: unsupported data type: " + to_string(static_cast<int>(_dataType)));
        return;
    }

    ignore(9);

    _width = readUint16();
    _height = readUint16();

    uint8_t bpp = readByte();
    _alpha = isRGBA() && bpp == 32;

    if ((isRGBA() && bpp != 24 && bpp != 32) ||
        (isGrayscale() && bpp != 8)) {

        throw runtime_error("Unsupported bits per pixel: " + to_string(bpp));
    }

    uint8_t descriptor = readByte();

    bool flipY = (descriptor & 0x10) != 0;
    if (flipY) {
        throw runtime_error("Vertically flipped images are not supported");
    }

    ignore(idLength);

    loadTexture();
}

bool TgaReader::isRGBA() const {
    return _dataType == TGADataType::RGBA || _dataType == TGADataType::RGBA_RLE;
}

bool TgaReader::isGrayscale() const {
    return _dataType == TGADataType::Grayscale;
}

void TgaReader::loadTexture() {
    bool cubeMap = _height / _width == 6;
    int layerCount = cubeMap ? 6 : 1;

    vector<Texture::Layer> layers;
    layers.reserve(layerCount);

    for (int i = 0; i < layerCount; ++i) {
        int w = _width;
        int h = cubeMap ? _width : _height;

        Texture::MipMap mipMap;
        mipMap.width = w;
        mipMap.height = h;
        mipMap.pixels = make_shared<ByteArray>(readPixels(w, h));

        Texture::Layer layer;
        layer.mipMaps.push_back(move(mipMap));
        layers.push_back(move(layer));
    }

    PixelFormat format = isGrayscale() ? PixelFormat::Grayscale : (_alpha ? PixelFormat::BGRA : PixelFormat::BGR);

    if (cubeMap) {
        prepareCubeMap(layers, format, format);
    }

    _texture = make_shared<Texture>(_resRef, getTextureProperties(_usage));
    _texture->init();
    _texture->bind();
    _texture->setPixels(_width, _height, format, move(layers));
}

ByteArray TgaReader::readPixels(int w, int h) {
    if (isRLE()) {
        return readPixelsRLE(w, h);
    }
    int dataSize = (isRGBA() ? (_alpha ? 4 : 3) : 1) * w * h;

    return _reader->getBytes(dataSize);
}

bool TgaReader::isRLE() const {
    return _dataType == TGADataType::RGBA_RLE;
}

ByteArray TgaReader::readPixelsRLE(int w, int h) {
    ByteArray result;

    int count = w * h;
    while (count > 0) {
        uint8_t code = readByte();
        int length = glm::min((code & 0x7f) + 1, count);

        count -= length;

        if (code & 0x80) {
            uint8_t b = readByte();
            uint8_t g = readByte();
            uint8_t r = readByte();
            uint8_t a = _alpha ? readByte() : 0;
            while (length--) {
                result.push_back(b);
                result.push_back(g);
                result.push_back(r);
                if (_alpha) {
                    result.push_back(a);
                }
            }
        } else {
            while (length--) {
                uint8_t b = readByte();
                uint8_t g = readByte();
                uint8_t r = readByte();
                uint8_t a = _alpha ? readByte() : 0;
                result.push_back(b);
                result.push_back(g);
                result.push_back(r);
                if (_alpha) {
                    result.push_back(a);
                }
            }
        }
    }

    return move(result);
}

} // namespace graphics

} // namespace reone
