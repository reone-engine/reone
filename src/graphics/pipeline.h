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

#include "framebuffer.h"
#include "options.h"
#include "renderbuffer.h"
#include "types.h"

namespace reone {

namespace graphics {

class GraphicsContext;
class IScene;
class Meshes;
class Shaders;
class Textures;

class Pipeline : boost::noncopyable {
public:
    Pipeline(
        GraphicsOptions options,
        GraphicsContext &graphicsContext,
        Meshes &meshes,
        Shaders &shaders,
        Textures &textures) :
        _options(std::move(options)),
        _graphicsContext(graphicsContext),
        _meshes(meshes),
        _shaders(shaders),
        _textures(textures) {
    }

    void init();

    std::shared_ptr<Texture> draw(IScene &scene, const glm::ivec2 &dim);

private:
    struct BlitFlags {
        static constexpr int color = 1;
        static constexpr int depth = 2;

        static constexpr int colorDepth = color | depth;
    };

    struct Vec2Hasher {
        size_t operator()(const glm::ivec2 &dim) const {
            std::hash<int> intHash;
            return intHash(dim.x) ^ intHash(dim.y);
        }
    };

    struct Attachments {
        std::shared_ptr<Texture> cbGBufferDiffuse;
        std::shared_ptr<Texture> cbGBufferLightmap;
        std::shared_ptr<Texture> cbGBufferEnvMap;
        std::shared_ptr<Texture> cbGBufferSelfIllum;
        std::shared_ptr<Texture> cbGBufferFeatures;
        std::shared_ptr<Texture> cbGBufferEyePos;
        std::shared_ptr<Texture> cbGBufferEyeNormal;
        std::shared_ptr<Texture> cbOpaqueGeometry1;
        std::shared_ptr<Texture> cbOpaqueGeometry2;
        std::shared_ptr<Texture> cbTransparentGeometry1;
        std::shared_ptr<Texture> cbTransparentGeometry2;
        std::shared_ptr<Texture> cbSSAO;
        std::shared_ptr<Texture> cbSSR;
        std::shared_ptr<Texture> cbPing;
        std::shared_ptr<Texture> cbPingHalf;
        std::shared_ptr<Texture> cbPong;
        std::shared_ptr<Texture> cbPongHalf;
        std::shared_ptr<Texture> cbOutput;

        std::shared_ptr<Renderbuffer> dbCommon;
        std::shared_ptr<Renderbuffer> dbCommonHalf;
        std::shared_ptr<Renderbuffer> dbGBuffer;
        std::shared_ptr<Texture> dbDirectionalLightShadows;
        std::shared_ptr<Texture> dbPointLightShadows;
        std::shared_ptr<Texture> dbOutput;

        std::shared_ptr<Framebuffer> fbPointLightShadows;
        std::shared_ptr<Framebuffer> fbDirectionalLightShadows;
        std::shared_ptr<Framebuffer> fbGBuffer;
        std::shared_ptr<Framebuffer> fbOpaqueGeometry;
        std::shared_ptr<Framebuffer> fbTransparentGeometry;
        std::shared_ptr<Framebuffer> fbSSAO;
        std::shared_ptr<Framebuffer> fbSSR;
        std::shared_ptr<Framebuffer> fbPing;
        std::shared_ptr<Framebuffer> fbPingHalf;
        std::shared_ptr<Framebuffer> fbPong;
        std::shared_ptr<Framebuffer> fbPongHalf;
        std::shared_ptr<Framebuffer> fbOutput;
    };

    GraphicsOptions _options;

    glm::mat4 _shadowLightSpace[kNumShadowLightSpace] {glm::mat4(1.0f)};
    glm::vec4 _shadowCascadeFarPlanes {glm::vec4(0.0f)};

    std::unordered_map<glm::ivec2, Attachments, Vec2Hasher> _attachments;

    // Services

    GraphicsContext &_graphicsContext;
    Meshes &_meshes;
    Shaders &_shaders;
    Textures &_textures;

    // END Services

    void initAttachments(glm::ivec2 extent);

    void computeLightSpaceMatrices(IScene &scene);

    void drawShadows(IScene &scene, Attachments &attachments);
    void drawOpaqueGeometry(IScene &scene, Attachments &attachments);
    void drawTransparentGeometry(IScene &scene, Attachments &attachments);
    void drawLensFlares(IScene &scene, Framebuffer &dst);
    void drawSSAO(IScene &scene, const glm::ivec2 &dim, Attachments &attachments);
    void drawSSR(IScene &scene, const glm::ivec2 &dim, Attachments &attachments);
    void drawCombineOpaque(IScene &scene, Attachments &attachments, Framebuffer &dst);
    void drawCombineOIT(Attachments &attachments, Framebuffer &dst);

    void drawGaussianBlur(const glm::ivec2 &dim, Texture &srcTexture, Framebuffer &dst, bool vertical, bool strong = false);
    void drawMedianFilter(const glm::ivec2 &dim, Texture &srcTexture, Framebuffer &dst, bool strong = false);
    void drawSSAOBlur(const glm::ivec2 &dim, Texture &srcTexture, Framebuffer &dst);
    void drawFXAA(const glm::ivec2 &dim, Texture &srcTexture, Framebuffer &dst);
    void drawSharpen(const glm::ivec2 &dim, Texture &srcTexture, Framebuffer &dst);

    void blitFramebuffer(const glm::ivec2 &dim, Framebuffer &src, int srcColorIdx, Framebuffer &dst, int dstColorIdx, int flags = BlitFlags::color);
};

} // namespace graphics

} // namespace reone
