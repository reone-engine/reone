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

#include "bikreader.h"

#include "../../audio/stream.h"
#include "../../common/logutil.h"
#include "../../common/streamreader.h"

#include "../movie.h"
#include "../videostream.h"

#ifdef R_ENABLE_MOVIE

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

#endif

namespace fs = boost::filesystem;

using namespace std;

using namespace reone::audio;

namespace reone {

namespace movie {

#ifdef R_ENABLE_MOVIE

class BinkVideoDecoder : public VideoStream {
public:
    BinkVideoDecoder(fs::path path) :
        _path(move(path)) {
    }

    ~BinkVideoDecoder() { deinit(); }

    void deinit() {
        if (_avFrameScaled) {
            av_frame_free(&_avFrameScaled);
        }
        if (_avFrame) {
            av_frame_free(&_avFrame);
        }
        if (_frameBuffer) {
            av_free(_frameBuffer);
            _frameBuffer = nullptr;
        }
        if (_swrContext) {
            swr_free(&_swrContext);
        }
        if (_swsContext) {
            sws_freeContext(_swsContext);
            _swsContext = nullptr;
        }
        if (_audioCodecCtx) {
            avcodec_free_context(&_audioCodecCtx);
        }
        if (_videoCodecCtx) {
            avcodec_free_context(&_videoCodecCtx);
        }
        if (_formatCtx) {
            avformat_close_input(&_formatCtx);
        }
    }

    void load() {
        if (avformat_open_input(&_formatCtx, _path.string().c_str(), nullptr, nullptr) != 0) {
            throw runtime_error("Failed to open BIK file: " + _path.string());
        }
        findStreams();

        // Video

        openCodec(_videoStreamIdx, &_videoCodecCtx);
        initFrames();
        initScalingContext();

        _width = _videoCodecCtx->width;
        _height = _videoCodecCtx->height;

        // Audio

        if (hasAudio()) {
            openCodec(_audioStreamIdx, &_audioCodecCtx);
            initResamplingContext();
            loadAudioStream();
            seekBeginning();
        }
    }

    void seek(float time) override {
        int64_t timestamp = streamTimestampFromTime(_videoStreamIdx, time);
        loadVideoFrame(timestamp);
    }

    std::shared_ptr<audio::AudioStream> audioStream() const { return _audioStream; }

private:
    fs::path _path;

    int _videoStreamIdx {-1};
    int _audioStreamIdx {-1};
    int64_t _lastPacketTimestamp {-1};

    AVFormatContext *_formatCtx {nullptr};
    AVCodecContext *_videoCodecCtx {nullptr};
    AVCodecContext *_audioCodecCtx {nullptr};
    SwsContext *_swsContext {nullptr};
    SwrContext *_swrContext {nullptr};
    AVFrame *_avFrame {nullptr};
    AVFrame *_avFrameScaled {nullptr};
    uint8_t *_frameBuffer {nullptr};

    std::shared_ptr<audio::AudioStream> _audioStream;

    void findStreams() {
        if (avformat_find_stream_info(_formatCtx, nullptr) != 0) {
            throw runtime_error("Failed to find BIK stream info");
        }
        for (uint32_t i = 0; i < _formatCtx->nb_streams; ++i) {
            AVCodecParameters *codecParams = _formatCtx->streams[i]->codecpar;
            switch (codecParams->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
                _videoStreamIdx = i;
                break;
            case AVMEDIA_TYPE_AUDIO:
                _audioStreamIdx = i;
                break;
            default:
                break;
            }
        }
        if (_videoStreamIdx == -1) {
            throw runtime_error("Video stream not found in BIK");
        }
    }

    void openCodec(int streamIdx, AVCodecContext **codecCtx) {
        AVCodecParameters *codecParams = _formatCtx->streams[streamIdx]->codecpar;
        AVCodec *codec = avcodec_find_decoder(codecParams->codec_id);
        if (!codec) {
            throw runtime_error("BIK codec not found");
        }
        *codecCtx = avcodec_alloc_context3(codec);
        if (avcodec_parameters_to_context(*codecCtx, codecParams) != 0) {
            throw runtime_error("Failed to copy BIK codec parameters");
        }
        if (avcodec_open2(*codecCtx, codec, nullptr) != 0) {
            throw runtime_error("Failed to open BIK codec");
        }
    }

    void initFrames() {
        _avFrame = av_frame_alloc();
        _avFrameScaled = av_frame_alloc();

        int bufSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, _videoCodecCtx->width, _videoCodecCtx->height, 32);
        _frameBuffer = static_cast<uint8_t *>(av_malloc(bufSize));
        av_image_fill_arrays(
            _avFrameScaled->data, _avFrameScaled->linesize,
            _frameBuffer,
            AV_PIX_FMT_RGB24,
            _videoCodecCtx->width, _videoCodecCtx->height,
            32);
    }

    void initScalingContext() {
        _swsContext = sws_getContext(
            _videoCodecCtx->width, _videoCodecCtx->height,
            _videoCodecCtx->pix_fmt,
            _videoCodecCtx->width, _videoCodecCtx->height,
            AV_PIX_FMT_RGB24,
            SWS_BILINEAR,
            nullptr, nullptr, nullptr);
    }

    void initResamplingContext() {
        _swrContext = swr_alloc_set_opts(
            nullptr,
            AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16, _audioCodecCtx->sample_rate,
            _audioCodecCtx->channel_layout, _audioCodecCtx->sample_fmt, _audioCodecCtx->sample_rate,
            0, nullptr);

        swr_init(_swrContext);
    }

    void loadVideoFrame(int64_t timestamp) {
        AVPacket packet;

        // Do nothing if last packet timestamp is greater or equals requested
        if (_lastPacketTimestamp != -1 && _lastPacketTimestamp >= timestamp) {
            return;
        }

        int ret;
        while ((ret = av_read_frame(_formatCtx, &packet)) >= 0) {
            if (packet.stream_index != _videoStreamIdx) {
                continue;
            }

            // Read packets until packet timestamp is greater or equals requested
            _lastPacketTimestamp = packet.pts;
            if (_lastPacketTimestamp < timestamp) {
                continue;
            }

            avcodec_send_packet(_videoCodecCtx, &packet);
            if ((ret = avcodec_receive_frame(_videoCodecCtx, _avFrame)) < 0) {
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                    continue;
                } else {
                    break;
                }
            }

            // Scale frame
            sws_scale(
                _swsContext,
                _avFrame->data, _avFrame->linesize, 0, _videoCodecCtx->height,
                _avFrameScaled->data, _avFrameScaled->linesize);

            // Save frame
            auto pixels = make_shared<ByteArray>(3ll * _videoCodecCtx->width * _videoCodecCtx->height);
            for (int y = 0; y < _videoCodecCtx->height; ++y) {
                int dstIdx = 3 * _videoCodecCtx->width * y;
                uint8_t *src = _avFrameScaled->data[0] + static_cast<long long>(y) * _avFrameScaled->linesize[0];
                memcpy(pixels->data() + dstIdx, src, 3ll * _videoCodecCtx->width);
            }
            _frame.pixels = move(pixels);

            break;
        }
        if (ret < 0) {
            _ended = true;
        }
        av_packet_unref(&packet);
    }

    void loadAudioStream() {
        _audioStream = make_shared<AudioStream>();

        AVPacket packet;
        int ret;
        while ((ret = av_read_frame(_formatCtx, &packet)) >= 0) {
            if (packet.stream_index != _audioStreamIdx) {
                continue;
            }
            avcodec_send_packet(_audioCodecCtx, &packet);
            while ((ret = avcodec_receive_frame(_audioCodecCtx, _avFrame)) >= 0) {
                // Resample frame
                int numSamples = swr_get_out_samples(_swrContext, _avFrame->nb_samples);
                int bufSize = av_samples_get_buffer_size(nullptr, 1, numSamples, AV_SAMPLE_FMT_S16, 1);
                ByteArray samples(bufSize);
                uint8_t *samplesPtr = reinterpret_cast<uint8_t *>(&samples[0]);
                swr_convert(
                    _swrContext,
                    &samplesPtr, numSamples,
                    const_cast<const uint8_t **>(&_avFrame->extended_data[0]), _avFrame->nb_samples);

                // Save frame
                AudioStream::Frame frame;
                frame.format = AudioFormat::Mono16;
                frame.sampleRate = _audioCodecCtx->sample_rate;
                frame.samples = move(samples);
                _audioStream->add(move(frame));
            }
            if (ret < 0) {
                if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                    continue;
                } else {
                    break;
                }
            }
        }
        av_packet_unref(&packet);
    }

    void seekBeginning() {
        av_seek_frame(_formatCtx, -1, 0, AVSEEK_FLAG_ANY);
    }

    int64_t streamTimestampFromTime(int streamIdx, float time) {
        auto micros = static_cast<int64_t>(1e6f * time);
        return av_rescale_q(micros, AVRational {1, AV_TIME_BASE}, _formatCtx->streams[streamIdx]->time_base);
    }

    float timeFromStreamTimestamp(int streamIdx, int64_t timestamp) {
        int64_t micros = av_rescale_q(timestamp, _formatCtx->streams[streamIdx]->time_base, AVRational {1, AV_TIME_BASE});
        return micros / 1e6f;
    }

    inline bool hasAudio() const {
        return _audioStreamIdx != -1;
    }
};

#endif

void BikReader::load() {
#ifdef R_ENABLE_MOVIE
    if (!fs::exists(_path)) {
        throw runtime_error("BIK: file not found: " + _path.string());
    }

    auto decoder = make_shared<BinkVideoDecoder>(_path);
    decoder->load();

    _movie = make_shared<Movie>(_graphicsContext, _meshes, _shaders, _textures, _uniforms, _audioPlayer);
    _movie->setVideoStream(decoder);
    _movie->setAudioStream(decoder->audioStream());
    _movie->init();
#endif
}

} // namespace movie

} // namespace reone
