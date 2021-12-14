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

#include "control.h"

#include "../../graphics/context.h"
#include "../../graphics/mesh.h"
#include "../../graphics/meshes.h"
#include "../../graphics/renderbuffer.h"
#include "../../graphics/shaders.h"
#include "../../graphics/texture.h"
#include "../../graphics/textures.h"
#include "../../graphics/textureutil.h"
#include "../../graphics/types.h"

#include "../graphs.h"
#include "../node/camera.h"

using namespace std;

using namespace reone::graphics;

namespace reone {

namespace scene {

void ControlRenderPipeline::init() {
    _geometry1 = make_shared<Framebuffer>();
    _geometry1->init();

    _geometry2 = make_shared<Framebuffer>();
    _geometry2->init();
}

void ControlRenderPipeline::prepareFor(const glm::ivec4 &extent) {
    AttachmentsId attachmentsId {extent};
    if (_attachments.count(attachmentsId) > 0) {
        return;
    }

    auto colorBuffer1 = make_unique<Texture>("color1", getTextureProperties(TextureUsage::ColorBufferMultisample));
    colorBuffer1->clearPixels(extent[2], extent[3], PixelFormat::RGBA);
    colorBuffer1->init();

    auto colorBuffer2 = make_unique<Texture>("color2", getTextureProperties(TextureUsage::ColorBuffer));
    colorBuffer2->clearPixels(extent[2], extent[3], PixelFormat::RGBA);
    colorBuffer2->init();

    auto depthBuffer1 = make_unique<Renderbuffer>(true);
    depthBuffer1->clearPixels(extent[2], extent[3], PixelFormat::Depth);
    depthBuffer1->init();

    auto depthBuffer2 = make_unique<Renderbuffer>();
    depthBuffer2->clearPixels(extent[2], extent[3], PixelFormat::Depth);
    depthBuffer2->init();

    Attachments attachments {move(colorBuffer1), move(colorBuffer2), move(depthBuffer1), move(depthBuffer2)};
    _attachments.insert(make_pair(attachmentsId, move(attachments)));
}

void ControlRenderPipeline::render(const string &sceneName, const glm::ivec4 &extent, const glm::ivec2 &offset) {
    AttachmentsId attachmentsId {extent};
    auto maybeAttachments = _attachments.find(attachmentsId);
    if (maybeAttachments == _attachments.end()) {
        return;
    }
    auto &attachments = maybeAttachments->second;
    SceneGraph &sceneGraph = _sceneGraphs.get(sceneName);

    // Set uniforms prototype

    shared_ptr<CameraSceneNode> camera(sceneGraph.activeCamera());

    auto &uniformsPrototype = sceneGraph.uniformsPrototype();
    uniformsPrototype.general = GeneralUniforms();
    uniformsPrototype.general.projection = camera->projection();
    uniformsPrototype.general.view = camera->view();
    uniformsPrototype.general.cameraPosition = camera->absoluteTransform()[3];

    // Draw to multi-sampled framebuffer

    glm::ivec4 oldViewport(_context.viewport());
    _context.setViewport(glm::ivec4(0, 0, extent[2], extent[3]));

    bool oldDepthTest = _context.isDepthTestEnabled();
    _context.setDepthTestEnabled(true);

    _context.bindFramebuffer(_geometry1);
    _geometry1->attachColor(*attachments.colorBuffer1);
    _geometry1->attachDepth(*attachments.depthBuffer1);

    _context.clear(ClearBuffers::colorDepth);
    sceneGraph.draw();

    // Blit multi-sampled framebuffer to normal

    _context.bindFramebuffer(_geometry2);
    _geometry2->attachColor(*attachments.colorBuffer2);
    _geometry2->attachDepth(*attachments.depthBuffer2);
    _geometry1->blitTo(*_geometry2, extent[2], extent[3]);

    // Draw control

    _context.setDepthTestEnabled(oldDepthTest);
    _context.setViewport(oldViewport);
    _context.unbindFramebuffer();
    _context.bindTexture(0, attachments.colorBuffer2);

    glm::mat4 projection(glm::ortho(
        0.0f,
        static_cast<float>(oldViewport[2]),
        static_cast<float>(oldViewport[3]),
        0.0f));

    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, glm::vec3(extent[0] + offset.x, extent[1] + offset.y, 0.0f));
    transform = glm::scale(transform, glm::vec3(extent[2], extent[3], 1.0f));

    auto &uniforms = _shaders.uniforms();
    uniforms.general = GeneralUniforms();
    uniforms.general.projection = move(projection);
    uniforms.general.model = move(transform);

    _context.useShaderProgram(_shaders.gui());
    _shaders.refreshUniforms();
    _meshes.quad().draw();
}

} // namespace scene

} // namespace reone
