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

/** @file
 *  TGA writer. Implementation adapted from libtga by Matthias Brueckner.
 */

#include "tgawriter.h"

#include "../dxtutil.h"
#include "../texture.h"

using namespace std;

namespace fs = boost::filesystem;

namespace reone {

namespace graphics {

static constexpr int kHeaderSize = 18;

TgaWriter::TgaWriter(shared_ptr<Texture> texture) :
    _texture(move(texture)) {
}

void TgaWriter::save(ostream &out, bool compress) {
    // Image ID, color-mapped images, RLE and image orientation are not supported

    int width = _texture->width();
    int totalHeight = static_cast<int>(_texture->layers().size()) * _texture->height();

    TGADataType dataType;
    int depth;
    vector<uint8_t> pixels(getTexturePixels(compress, dataType, depth));

    // Write Header

    uint8_t header[kHeaderSize] {0};
    header[0] = 0; // ID length
    header[1] = 0; // color map type
    header[2] = static_cast<uint8_t>(dataType);
    memset(header + 4, 0, 5);
    header[8] = 0;                    // X origin (lo)
    header[9] = 0;                    // X origin (hi)
    header[10] = 0;                   // Y origin (lo)
    header[11] = 0;                   // Y origin (hi)
    header[12] = width % 256;         // width (lo)
    header[13] = width / 256;         // width (hi)
    header[14] = totalHeight % 256;   // height (lo)
    header[15] = totalHeight / 256;   // height (hi)
    header[16] = depth;               // pixel size
    header[17] = depth == 32 ? 8 : 0; // image descriptor;
    out.write(reinterpret_cast<char *>(header), kHeaderSize);

    // Write Scanlines

    int scanlineSize = width * depth / 8;
    if (depth >= 24 && compress) {
        for (int wrote = 0; wrote < totalHeight; ++wrote) {
            int offset = wrote * scanlineSize;
            writeRLE(&pixels[offset], depth, out);
        }
    } else {
        out.write(reinterpret_cast<char *>(&pixels[0]), static_cast<size_t>(totalHeight) * scanlineSize);
    }
}

vector<uint8_t> TgaWriter::getTexturePixels(bool compress, TGADataType &dataType, int &depth) const {
    vector<uint8_t> result;

    switch (_texture->pixelFormat()) {
    case PixelFormat::Grayscale:
        dataType = TGADataType::Grayscale;
        depth = 8;
        break;
    case PixelFormat::RGB:
    case PixelFormat::BGR:
    case PixelFormat::DXT1:
        dataType = compress ? TGADataType::RGBA_RLE : TGADataType::RGBA;
        depth = 24;
        break;
    case PixelFormat::RGBA:
    case PixelFormat::BGRA:
    case PixelFormat::DXT5:
        dataType = compress ? TGADataType::RGBA_RLE : TGADataType::RGBA;
        depth = 32;
        break;
    default:
        throw runtime_error("Unsupported texture pixel format: " + to_string(static_cast<int>(_texture->pixelFormat())));
    }

    int numLayers = static_cast<int>(_texture->layers().size());
    int numPixels = _texture->width() * _texture->height();
    int numPixelsTotal = numLayers * numPixels;
    result.resize(static_cast<size_t>(numPixelsTotal) * depth / 8);
    uint8_t *pixels = &result[0];

    for (int i = 0; i < numLayers; ++i) {
        const Texture::MipMap &mipMap = _texture->layers()[i].mipMaps.front();
        const uint8_t *mipMapPtr = reinterpret_cast<const uint8_t *>(mipMap.pixels->data());

        switch (_texture->pixelFormat()) {
        case PixelFormat::Grayscale:
            memcpy(pixels, mipMapPtr, numPixels);
            break;
        case PixelFormat::RGB:
            for (int j = 0; j < numPixels; ++j) {
                *(pixels++) = mipMapPtr[2];
                *(pixels++) = mipMapPtr[1];
                *(pixels++) = mipMapPtr[0];
                mipMapPtr += 3;
            }
            break;
        case PixelFormat::RGBA:
            for (int j = 0; j < numPixels; ++j) {
                *(pixels++) = mipMapPtr[2];
                *(pixels++) = mipMapPtr[1];
                *(pixels++) = mipMapPtr[0];
                *(pixels++) = mipMapPtr[3];
                mipMapPtr += 4;
            }
            break;
        case PixelFormat::BGR:
            memcpy(pixels, mipMapPtr, 3ll * numPixels);
            break;
        case PixelFormat::BGRA:
            memcpy(pixels, mipMapPtr, 4ll * numPixels);
            break;
        case PixelFormat::DXT1: {
            vector<uint32_t> decompPixels(numPixels);
            decompressDXT1(mipMap.width, mipMap.height, mipMapPtr, &decompPixels[0]);
            uint32_t *decompPtr = &decompPixels[0];
            for (int j = 0; j < numPixels; ++j) {
                uint32_t rgba = *(decompPtr++);
                *(pixels++) = (rgba >> 8) & 0xff;
                *(pixels++) = (rgba >> 16) & 0xff;
                *(pixels++) = (rgba >> 24) & 0xff;
            }
            break;
        }
        case PixelFormat::DXT5: {
            vector<uint32_t> decompPixels(numPixels);
            decompressDXT5(mipMap.width, mipMap.height, mipMapPtr, &decompPixels[0]);
            uint32_t *decompPtr = &decompPixels[0];
            for (int j = 0; j < numPixels; ++j) {
                uint32_t rgba = *(decompPtr++);
                *(pixels++) = (rgba >> 8) & 0xff;
                *(pixels++) = (rgba >> 16) & 0xff;
                *(pixels++) = (rgba >> 24) & 0xff;
                *(pixels++) = rgba & 0xff;
            }
            break;
        }
        default:
            break;
        }
    }

    return move(result);
}

void TgaWriter::writeRLE(uint8_t *pixels, int depth, ostream &out) {
    uint8_t *from = pixels;
    uint8_t repeat = 0, direct = 0;
    int bytes = depth / 8;

    for (int x = 1; x < _texture->width(); ++x) {
        if (memcpy(pixels, pixels + bytes, bytes)) {
            if (repeat) {
                out.put(128 + repeat);
                out.write(reinterpret_cast<char *>(from), bytes);
                from = pixels + bytes;
                repeat = 0;
                direct = 0;
            } else {
                ++direct;
            }
        } else {
            if (direct) {
                out.put(direct - 1);
                out.write(reinterpret_cast<char *>(from), bytes * static_cast<size_t>(direct));
                from = pixels;
                direct = 0;
                repeat = 1;
            } else {
                ++repeat;
            }
        }
        if (repeat == 128) {
            out.put(static_cast<char>(255));
            out.write(reinterpret_cast<char *>(from), bytes);
            from = pixels + bytes;
            direct = 0;
            repeat = 0;
        } else if (direct == 128) {
            out.put(127);
            out.write(reinterpret_cast<char *>(from), bytes * static_cast<size_t>(direct));
            from = pixels + bytes;
            direct = 0;
            repeat = 0;
        }
        pixels += bytes;
    }

    if (repeat > 0) {
        out.put(128 + repeat);
        out.write(reinterpret_cast<char *>(from), bytes);
    } else {
        out.put(direct);
        out.write(reinterpret_cast<char *>(from), bytes * static_cast<size_t>(direct + 1));
    }
}

void TgaWriter::save(const fs::path &path, bool compress) {
    fs::ofstream tga(path, ios::binary);
    save(tga, compress);
}

} // namespace graphics

} // namespace reone
