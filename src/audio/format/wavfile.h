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

#pragma once

#include "../../resource/binfile.h"

#include "../stream.h"

namespace reone {

namespace audio {

enum class WavAudioFormat {
    PCM = 1,
    IMAADPCM = 0x11
};

class WavFile : public resource::BinaryFile {
public:
    WavFile();

    std::shared_ptr<AudioStream> stream() const;

private:
    struct ChunkHeader {
        std::string id;
        uint32_t size { 0 };
    };

    struct IMA {
        int16_t lastSample { 0 };
        int16_t stepIndex { 0 };
    };

    WavAudioFormat _audioFormat { WavAudioFormat::PCM };
    uint16_t _channelCount { 0 };
    uint32_t _sampleRate { 0 };
    uint16_t _blockAlign { 0 };
    uint16_t _bitsPerSample { 0 };
    IMA _ima[2];
    std::shared_ptr<AudioStream> _stream;

    void doLoad() override;

    int16_t getIMASample(int channel, uint8_t nibble);
    void getIMASamples(int channel, uint8_t nibbles, int16_t &sample1, int16_t &sample2);
    void loadData(ChunkHeader chunk);
    void loadFormat(ChunkHeader chunk);
    void loadIMAADPCM(uint32_t chunkSize);
    void loadPCM(uint32_t chunkSize);
    bool readChunkHeader(ChunkHeader &chunk);

    AudioFormat getAudioFormat() const;
};

} // namespace audio

} // namespace reone
