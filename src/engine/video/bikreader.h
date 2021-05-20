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

#include <boost/filesystem/path.hpp>

#include "../audio/types.h"
#include "../graphics/mesh/meshes.h"
#include "../graphics/shader/shaders.h"

namespace reone {

namespace video {

class Video;

/**
 * Encapsulates loading Bink Video files using FFmpeg.
 *
 * http://dranger.com/ffmpeg/ffmpeg.html
 */
class BikReader {
public:
    BikReader(boost::filesystem::path path, graphics::Shaders *shaders, graphics::Meshes *meshes);

    void load();

    std::shared_ptr<Video> video() const { return _video; }

private:
    boost::filesystem::path _path;
    graphics::Shaders *_shaders;
    graphics::Meshes *_meshes;

    std::shared_ptr<Video> _video;
};

} // namespace video

} // namespace reone
