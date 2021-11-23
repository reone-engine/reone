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

#include "mesh.h"

#include "../../common/logutil.h"
#include "../../common/randomutil.h"
#include "../../graphics/context.h"
#include "../../graphics/features.h"
#include "../../graphics/materials.h"
#include "../../graphics/mesh/mesh.h"
#include "../../graphics/pbribl.h"
#include "../../graphics/texture/texture.h"
#include "../../graphics/texture/textures.h"

#include "../graph.h"

#include "camera.h"
#include "light.h"
#include "model.h"

using namespace std;

using namespace reone::graphics;

namespace reone {

namespace scene {

static constexpr float kUvAnimationSpeed = 250.0f;

static bool g_debugWalkmesh = false;

MeshSceneNode::MeshSceneNode(
    const ModelSceneNode &model,
    shared_ptr<ModelNode> modelNode,
    SceneGraph &sceneGraph,
    Context &context,
    Features &features,
    Materials &materials,
    Meshes &meshes,
    PBRIBL &pbrIbl,
    Shaders &shaders,
    Textures &textures) :
    ModelNodeSceneNode(
        modelNode,
        SceneNodeType::Mesh,
        sceneGraph,
        context,
        meshes,
        shaders),
    _model(model),
    _features(features),
    _materials(materials),
    _pbrIbl(pbrIbl),
    _textures(textures) {

    _point = false;
    _alpha = _modelNode->alpha().getByFrameOrElse(0, 1.0f);
    _selfIllumColor = _modelNode->selfIllumColor().getByFrameOrElse(0, glm::vec3(0.0f));

    initTextures();
}

void MeshSceneNode::initTextures() {
    shared_ptr<ModelNode::TriangleMesh> mesh(_modelNode->mesh());
    if (!mesh)
        return;

    _nodeTextures.diffuse = mesh->diffuseMap;
    _nodeTextures.lightmap = mesh->lightmap;
    _nodeTextures.bumpmap = mesh->bumpmap;

    refreshMaterial();
    refreshAdditionalTextures();
}

void MeshSceneNode::refreshMaterial() {
    _material = Material();

    if (_nodeTextures.diffuse) {
        shared_ptr<Material> material(_materials.get(_nodeTextures.diffuse->name()));
        if (material) {
            _material = *material;
        }
    }
}

void MeshSceneNode::refreshAdditionalTextures() {
    _nodeTextures.envmap.reset();
    _nodeTextures.bumpmap.reset();

    if (!_nodeTextures.diffuse)
        return;

    const Texture::Features &features = _nodeTextures.diffuse->features();
    if (!features.envmapTexture.empty()) {
        _nodeTextures.envmap = _textures.get(features.envmapTexture, TextureUsage::EnvironmentMap);
    } else if (!features.bumpyShinyTexture.empty()) {
        _nodeTextures.envmap = _textures.get(features.bumpyShinyTexture, TextureUsage::EnvironmentMap);
    }
    if (!features.bumpmapTexture.empty()) {
        _nodeTextures.bumpmap = _textures.get(features.bumpmapTexture, TextureUsage::Bumpmap);
    }
}

void MeshSceneNode::update(float dt) {
    SceneNode::update(dt);

    shared_ptr<ModelNode::TriangleMesh> mesh(_modelNode->mesh());
    if (mesh) {
        updateUVAnimation(dt, *mesh);
        updateBumpmapAnimation(dt, *mesh);
        updateDanglyMeshAnimation(dt, *mesh);
    }
}

void MeshSceneNode::updateUVAnimation(float dt, const ModelNode::TriangleMesh &mesh) {
    if (mesh.uvAnimation.dir.x != 0.0f || mesh.uvAnimation.dir.y != 0.0f) {
        _uvOffset += kUvAnimationSpeed * mesh.uvAnimation.dir * dt;
        _uvOffset -= glm::floor(_uvOffset);
    }
}

void MeshSceneNode::updateBumpmapAnimation(float dt, const ModelNode::TriangleMesh &mesh) {
    if (!_nodeTextures.bumpmap)
        return;

    const Texture::Features &features = _nodeTextures.bumpmap->features();
    if (features.procedureType == Texture::ProcedureType::Cycle) {
        int frameCount = features.numX * features.numY;
        float length = frameCount / static_cast<float>(features.fps);
        _bumpmapTime = glm::min(_bumpmapTime + dt, length);
        _bumpmapFrame = static_cast<int>(glm::round((frameCount - 1) * (_bumpmapTime / length)));
        if (_bumpmapTime == length) {
            _bumpmapTime = 0.0f;
        }
    }
}

void MeshSceneNode::updateDanglyMeshAnimation(float dt, const ModelNode::TriangleMesh &mesh) {
    shared_ptr<ModelNode::DanglyMesh> danglyMesh(mesh.danglyMesh);
    if (!danglyMesh)
        return;

    bool forceApplied = glm::length2(_danglymeshAnimation.force) > 0.0f;
    if (forceApplied) {
        // When force is applied, stride in the opposite direction from the applied force
        glm::vec3 strideDir(-_danglymeshAnimation.force);
        glm::vec3 maxStride(danglyMesh->displacement);
        _danglymeshAnimation.stride = glm::clamp(_danglymeshAnimation.stride + danglyMesh->period * strideDir * dt, -maxStride, maxStride);
    } else {
        // When force is not applied, gradually nullify stride
        float strideMag2 = glm::length2(_danglymeshAnimation.stride);
        if (strideMag2 > 0.0f) {
            glm::vec3 strideDir(-_danglymeshAnimation.stride);
            _danglymeshAnimation.stride += danglyMesh->period * strideDir * dt;
            if ((strideDir.x > 0.0f && _danglymeshAnimation.stride.x > 0.0f) || (strideDir.x < 0.0f && _danglymeshAnimation.stride.x < 0.0f)) {
                _danglymeshAnimation.stride.x = 0.0f;
            }
            if ((strideDir.y > 0.0f && _danglymeshAnimation.stride.y > 0.0f) || (strideDir.y < 0.0f && _danglymeshAnimation.stride.y < 0.0f)) {
                _danglymeshAnimation.stride.y = 0.0f;
            }
            if ((strideDir.z > 0.0f && _danglymeshAnimation.stride.z > 0.0f) || (strideDir.z < 0.0f && _danglymeshAnimation.stride.z < 0.0f)) {
                _danglymeshAnimation.stride.z = 0.0f;
            }
        }
    }
}

bool MeshSceneNode::shouldRender() const {
    if (g_debugWalkmesh)
        return _modelNode->isAABBMesh();

    shared_ptr<ModelNode::TriangleMesh> mesh(_modelNode->mesh());
    if (!mesh || !mesh->render || _modelNode->alpha().getByFrameOrElse(0, 1.0f) == 0.0f)
        return false;

    return !_modelNode->isAABBMesh();
}

bool MeshSceneNode::shouldCastShadows() const {
    // Skin nodes must not cast shadows
    if (_modelNode->isSkinMesh())
        return false;

    // Meshless nodes must not cast shadows
    shared_ptr<ModelNode::TriangleMesh> mesh(_modelNode->mesh());
    if (!mesh)
        return false;

    return mesh->shadow;
}

bool MeshSceneNode::isTransparent() const {
    shared_ptr<ModelNode::TriangleMesh> mesh(_modelNode->mesh());
    if (!mesh)
        return false; // Meshless nodes are opaque

    // Character models are opaque
    if (_model.model().classification() == MdlClassification::character)
        return false;

    // Model nodes with alpha less than 1.0 are transparent
    if (_alpha < 1.0f)
        return true;

    // Model nodes without a diffuse texture are opaque
    if (!_nodeTextures.diffuse)
        return false;

    // Model nodes with transparency hint greater than 0 are transparent
    if (mesh->transparency > 0)
        return true;

    // Model nodes with additive diffuse texture are opaque
    if (_nodeTextures.diffuse->isAdditive())
        return true;

    // Model nodes with an environment map or a bump map are opaque
    if (_nodeTextures.envmap || _nodeTextures.bumpmap)
        return false;

    // Model nodes with RGB diffuse textures are opaque
    PixelFormat format = _nodeTextures.diffuse->pixelFormat();
    if (format == PixelFormat::RGB || format == PixelFormat::BGR || format == PixelFormat::DXT1)
        return false;

    return true;
}

static bool isLightingEnabledByUsage(ModelUsage usage) {
    return usage != ModelUsage::Projectile;
}

bool MeshSceneNode::isSelfIlluminated() const {
    return !_nodeTextures.lightmap && glm::dot(_selfIllumColor, _selfIllumColor) > 0.0f;
}

static bool isReceivingShadows(const ModelSceneNode &model, const MeshSceneNode &modelNode) {
    // Only room models receive shadows, unless model node is self-illuminated
    return model.usage() == ModelUsage::Room && !modelNode.isSelfIlluminated();
}

void MeshSceneNode::drawSingle(bool shadowPass) {
    shared_ptr<ModelNode::TriangleMesh> mesh(_modelNode->mesh());
    if (!mesh)
        return;

    // Setup shaders

    ShaderUniforms uniforms(_sceneGraph.uniformsPrototype());
    uniforms.combined.general.model = _absTransform;
    uniforms.combined.general.alpha = _alpha;
    uniforms.combined.general.ambientColor = glm::vec4(_sceneGraph.ambientLightColor(), 1.0f);

    ShaderProgram program;

    if (shadowPass) {
        program = ShaderProgram::SimpleDepth;

    } else {
        if (!_nodeTextures.diffuse) {
            program = ShaderProgram::ModelBlinnPhongDiffuseless;
        } else if (_features.isEnabled(Feature::PBR)) {
            program = ShaderProgram::ModelPBR;
        } else {
            program = ShaderProgram::ModelBlinnPhong;
        }

        if (_nodeTextures.diffuse) {
            uniforms.combined.featureMask |= UniformFeatureFlags::diffuse;
        }

        if (_nodeTextures.envmap) {
            uniforms.combined.featureMask |= UniformFeatureFlags::envmap;

            if (_features.isEnabled(Feature::PBR)) {
                bool derived = _pbrIbl.contains(_nodeTextures.envmap.get());
                if (derived) {
                    uniforms.combined.featureMask |= UniformFeatureFlags::pbrIbl;
                }
            }
        }

        if (_nodeTextures.lightmap && !_features.isEnabled(Feature::DynamicRoomLighting)) {
            uniforms.combined.featureMask |= UniformFeatureFlags::lightmap;
        }

        if (_nodeTextures.bumpmap) {
            if (_nodeTextures.bumpmap->isGrayscale()) {
                uniforms.combined.featureMask |= UniformFeatureFlags::bumpmap;
                uniforms.combined.bumpmaps.gridSize = glm::vec2(_nodeTextures.bumpmap->features().numX, _nodeTextures.bumpmap->features().numY);
                uniforms.combined.bumpmaps.scaling = _nodeTextures.bumpmap->features().bumpMapScaling;
                uniforms.combined.bumpmaps.frame = _bumpmapFrame;
            } else {
                uniforms.combined.featureMask |= UniformFeatureFlags::normalmap;
            }
        }

        bool receivesShadows = isReceivingShadows(_model, *this);
        if (receivesShadows) {
            uniforms.combined.featureMask |= UniformFeatureFlags::shadows;
        }

        if (mesh->skin) {
            uniforms.combined.featureMask |= UniformFeatureFlags::skeletal;

            // Offset bone matrices by 1 to account for negative bone indices
            uniforms.skeletal->bones[0] = glm::mat4(1.0f);
            for (size_t i = 1; i < kMaxBones; ++i) {
                glm::mat4 tmp(1.0f);
                if (i < 1 + mesh->skin->boneNodeNumber.size()) {
                    uint16_t nodeNumber = mesh->skin->boneNodeNumber[i - 1];
                    if (nodeNumber != 0xffff) {
                        shared_ptr<ModelNodeSceneNode> bone(_model.getNodeByNumber(nodeNumber));
                        if (bone && bone->type() == SceneNodeType::Mesh) {
                            tmp = _modelNode->absoluteTransformInverse() *
                                  _model.absoluteTransformInverse() *
                                  bone->absoluteTransform() *
                                  mesh->skin->boneMatrices[mesh->skin->boneSerial[i - 1]];
                        }
                    }
                }
                uniforms.skeletal->bones[i] = move(tmp);
            }
        }

        if (isSelfIlluminated()) {
            uniforms.combined.featureMask |= UniformFeatureFlags::selfIllum;
            uniforms.combined.general.selfIllumColor = glm::vec4(_selfIllumColor, 1.0f);
        }
        if (isLightingEnabled()) {
            const vector<LightSceneNode *> &lights = _sceneGraph.closestLights();

            uniforms.combined.featureMask |= UniformFeatureFlags::lighting;
            uniforms.combined.material.ambient = glm::vec4(mesh->ambient, 1.0f);
            uniforms.combined.material.diffuse = glm::vec4(mesh->diffuse, 1.0f);
            uniforms.combined.material.shininess = _material.shininess;
            uniforms.combined.material.metallic = _material.metallic;
            uniforms.combined.material.roughness = _material.roughness;
            uniforms.lighting->lightCount = static_cast<int>(lights.size());

            for (size_t i = 0; i < lights.size(); ++i) {
                glm::vec4 position(lights[i]->absoluteTransform()[3]);
                position.w = lights[i]->isDirectional() ? 0.0f : 1.0f;

                ShaderLight &shaderLight = uniforms.lighting->lights[i];
                shaderLight.position = move(position);
                shaderLight.color = glm::vec4(lights[i]->color(), 1.0f);
                shaderLight.multiplier = lights[i]->multiplier() * (1.0f - lights[i]->fadeFactor());
                shaderLight.radius = lights[i]->radius();
                shaderLight.ambientOnly = static_cast<int>(lights[i]->modelNode().light()->ambientOnly);
            }
        }

        if (_nodeTextures.diffuse) {
            uniforms.combined.general.uvOffset = _uvOffset;

            float waterAlpha = _nodeTextures.diffuse->features().waterAlpha;
            if (waterAlpha != -1.0f) {
                uniforms.combined.featureMask |= UniformFeatureFlags::water;
                uniforms.combined.general.waterAlpha = waterAlpha;
            }
        }

        if (_sceneGraph.isFogEnabled() && _model.model().isAffectedByFog()) {
            uniforms.combined.featureMask |= UniformFeatureFlags::fog;
            uniforms.combined.general.fogNear = _sceneGraph.fogNear();
            uniforms.combined.general.fogFar = _sceneGraph.fogFar();
            uniforms.combined.general.fogColor = glm::vec4(_sceneGraph.fogColor(), 1.0f);
        }

        shared_ptr<ModelNode::DanglyMesh> danglyMesh(mesh->danglyMesh);
        if (danglyMesh) {
            uniforms.combined.featureMask |= UniformFeatureFlags::danglymesh;
            uniforms.danglymesh->stride = glm::vec4(_danglymeshAnimation.stride, 0.0f);
            uniforms.danglymesh->displacement = danglyMesh->displacement;
            for (size_t i = 0; i < danglyMesh->constraints.size(); ++i) {
                uniforms.danglymesh->constraints[i / 4][i % 4] = danglyMesh->constraints[i].multiplier;
            }
        }
    }

    _shaders.activate(program, uniforms);

    bool additive = false;

    // Setup textures

    if (_nodeTextures.diffuse) {
        _context.setActiveTextureUnit(TextureUnits::diffuseMap);
        _nodeTextures.diffuse->bind();
        additive = _nodeTextures.diffuse->isAdditive();
    }
    if (_nodeTextures.lightmap) {
        _context.setActiveTextureUnit(TextureUnits::lightmap);
        _nodeTextures.lightmap->bind();
    }
    if (_nodeTextures.envmap) {
        _context.setActiveTextureUnit(TextureUnits::environmentMap);
        _nodeTextures.envmap->bind();

        PBRIBL::Derived derived;
        if (_pbrIbl.getDerived(_nodeTextures.envmap.get(), derived)) {
            _context.setActiveTextureUnit(TextureUnits::irradianceMap);
            derived.irradianceMap->bind();
            _context.setActiveTextureUnit(TextureUnits::prefilterMap);
            derived.prefilterMap->bind();
            _context.setActiveTextureUnit(TextureUnits::brdfLookup);
            derived.brdfLookup->bind();
        }
    }
    if (_nodeTextures.bumpmap) {
        _context.setActiveTextureUnit(TextureUnits::bumpMap);
        _nodeTextures.bumpmap->bind();
    }

    BlendMode oldBlendMode(_context.blendMode());
    if (additive) {
        _context.setBlendMode(BlendMode::Add);
    }
    mesh->mesh->draw();
    _context.setBlendMode(oldBlendMode);
}

bool MeshSceneNode::isLightingEnabled() const {
    if (!isLightingEnabledByUsage(_model.usage()))
        return false;

    // Lighting is disabled for lightmapped models, unless dynamic room lighting is enabled
    if (_nodeTextures.lightmap && !_features.isEnabled(Feature::DynamicRoomLighting))
        return false;

    // Lighting is disabled for self-illuminated model nodes, e.g. sky boxes
    if (isSelfIlluminated())
        return false;

    // Lighting is disabled when diffuse texture is additive
    if (_nodeTextures.diffuse && _nodeTextures.diffuse->isAdditive())
        return false;

    return true;
}

void MeshSceneNode::setAppliedForce(glm::vec3 force) {
    if (_modelNode->isDanglyMesh()) {
        // Convert force from world to object space
        _danglymeshAnimation.force = _absTransformInv * glm::vec4(force, 0.0f);
    }
}

void MeshSceneNode::setDiffuseTexture(const shared_ptr<Texture> &texture) {
    _nodeTextures.diffuse = texture;
    refreshMaterial();
    refreshAdditionalTextures();
}

} // namespace scene

} // namespace reone
