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

#include "../graphs.h"
#include "../node/camera.h"

using namespace std;

using namespace reone::graphics;

namespace reone {

namespace scene {

void ControlRenderPipeline::init() {
    _geometry.init();
}

void ControlRenderPipeline::prepareFor(const glm::ivec4 &extent) {
    AttachmentsId attachmentsId {extent};
    if (_attachments.count(attachmentsId) > 0) {
        return;
    }

    auto colorBuffer = make_unique<Texture>("color", getTextureProperties(TextureUsage::ColorBuffer));
    colorBuffer->init();
    colorBuffer->bind();
    colorBuffer->clearPixels(extent[2], extent[3], PixelFormat::RGBA);
    colorBuffer->unbind();

    auto depthBuffer = make_unique<Renderbuffer>();
    depthBuffer->init();
    depthBuffer->bind();
    depthBuffer->configure(extent[2], extent[3], PixelFormat::Depth);
    _context.unbindRenderbuffer();

    Attachments attachments {move(colorBuffer), move(depthBuffer)};
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

    ShaderUniforms uniforms(_shaders.defaultUniforms());
    uniforms.combined.general.projection = camera->projection();
    uniforms.combined.general.view = camera->view();
    uniforms.combined.general.cameraPosition = camera->absoluteTransform()[3];

    sceneGraph.setUniformsPrototype(move(uniforms));

    // Draw to framebuffer

    glm::ivec4 oldViewport(_context.viewport());
    _context.setViewport(glm::ivec4(0, 0, extent[2], extent[3]));

    bool oldDepthTest = _context.isDepthTestEnabled();
    _context.setDepthTestEnabled(true);

    _geometry.bind();
    _geometry.attachColor(*attachments.colorBuffer);
    _geometry.attachDepth(*attachments.depthBuffer);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    sceneGraph.draw();

    _context.unbindFramebuffer();
    _context.setDepthTestEnabled(oldDepthTest);
    _context.setViewport(oldViewport);

    // Draw control

    _context.setActiveTextureUnit(TextureUnits::diffuseMap);
    attachments.colorBuffer->bind();

    glm::mat4 projection(glm::ortho(
        0.0f,
        static_cast<float>(oldViewport[2]),
        static_cast<float>(oldViewport[3]),
        0.0f));

    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, glm::vec3(extent[0] + offset.x, extent[1] + offset.y, 0.0f));
    transform = glm::scale(transform, glm::vec3(extent[2], extent[3], 1.0f));

    uniforms = ShaderUniforms();
    uniforms.combined.general.projection = move(projection);
    uniforms.combined.general.model = move(transform);

    _shaders.activate(ShaderProgram::SimpleGUI, uniforms);
    _meshes.quad().draw();
}

} // namespace scene

} // namespace reone
