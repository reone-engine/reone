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

#include "tpcreader.h"

#include "../../common/streamutil.h"

#include "../textureutil.h"

#include "txireader.h"

using namespace std;

namespace fs = boost::filesystem;

namespace reone {

namespace graphics {

TpcReader::TpcReader(const string &resRef, TextureUsage usage) :
    BinaryReader(0), _resRef(resRef), _usage(usage) {
}

void TpcReader::doLoad() {
    uint32_t dataSize = readUint32();

    ignore(4);

    uint16_t width = readUint16();
    uint16_t height = readUint16();
    _encoding = static_cast<EncodingType>(readByte());
    _numMipMaps = readByte();

    bool cubemap = height / width == kNumCubeFaces;
    if (cubemap) {
        _width = width;
        _height = width;
        _numLayers = kNumCubeFaces;
    } else {
        _width = width;
        _height = height;
        _numLayers = 1;
    }

    if (dataSize > 0) {
        _dataSize = dataSize;
        _compressed = true;
    } else {
        int w, h;
        getMipMapSize(0, w, h);
        _dataSize = getMipMapDataSize(w, h);
    }

    loadLayers();
    loadFeatures();
    loadTexture();
}

void TpcReader::loadLayers() {
    seek(128);

    _layers.reserve(_numLayers);

    for (int i = 0; i < _numLayers; ++i) {
        auto pixels = make_shared<ByteArray>(_reader->getBytes(_dataSize));

        // Ignore mip maps
        for (int j = 1; j < _numMipMaps; ++j) {
            int w, h;
            getMipMapSize(j, w, h);
            _reader->ignore(getMipMapDataSize(w, h));
        }

        _layers.push_back(Texture::Layer {move(pixels)});
    }
}

void TpcReader::loadFeatures() {
    size_t pos = tell();
    if (pos >= _size) {
        return;
    }
    _txiData = _reader->getBytes(static_cast<int>(_size - pos));

    TxiReader txi;
    txi.load(wrap(_txiData));

    _features = txi.features();
}

void TpcReader::loadTexture() {
    _texture = make_shared<Texture>(_resRef, getTextureProperties(_usage));
    _texture->setPixels(_width, _height, getPixelFormat(), _layers);
    _texture->setFeatures(_features);
}

void TpcReader::getMipMapSize(int index, int &width, int &height) const {
    width = glm::max(1, _width >> index);
    height = glm::max(1, _height >> index);
}

int TpcReader::getMipMapDataSize(int width, int height) const {
    if (_compressed) {
        switch (_encoding) {
        case EncodingType::RGB:
            return glm::max(8, ((width + 3) / 4) * ((height + 3) / 4) * 8);
        case EncodingType::RGBA:
            return glm::max(16, ((width + 3) / 4) * ((height + 3) / 4) * 16);
        default:
            break;
        }
    } else {
        switch (_encoding) {
        case EncodingType::Grayscale:
            return width * height;
        case EncodingType::RGB:
            return 3 * width * height;
        case EncodingType::RGBA:
            return 4 * width * height;
        }
    }

    throw logic_error("Unable to compute TPC mip map size");
}

PixelFormat TpcReader::getPixelFormat() const {
    if (!_compressed) {
        switch (_encoding) {
        case EncodingType::Grayscale:
            return PixelFormat::Grayscale;
        case EncodingType::RGB:
            return PixelFormat::RGB;
        case EncodingType::RGBA:
            return PixelFormat::RGBA;
        default:
            throw logic_error("Unsupported uncompressed TPC encoding: " + to_string(static_cast<int>(_encoding)));
        }
    } else
        switch (_encoding) {
        case EncodingType::RGB:
            return PixelFormat::DXT1;
        case EncodingType::RGBA:
            return PixelFormat::DXT5;
        default:
            throw logic_error("Unsupported compressed TPC encoding: " + to_string(static_cast<int>(_encoding)));
        }
}

} // namespace graphics

} // namespace reone
