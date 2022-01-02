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

#include "emitter.h"

#include "../../common/randomutil.h"
#include "../../graphics/context.h"
#include "../../graphics/mesh.h"
#include "../../graphics/meshes.h"
#include "../../graphics/shaders.h"
#include "../../graphics/texture.h"
#include "../../graphics/textures.h"

#include "../graph.h"

#include "camera.h"
#include "particle.h"

using namespace std;

using namespace reone::graphics;

namespace reone {

namespace scene {

static constexpr float kMotionBlurStrength = 0.25f;
static constexpr float kProjectileSpeed = 16.0f;

void EmitterSceneNode::init() {
    _birthrate = _modelNode->birthrate().getByFrameOrElse(0, 0.0f);
    _lifeExpectancy = _modelNode->lifeExp().getByFrameOrElse(0, 0.0f);
    _size.x = _modelNode->xSize().getByFrameOrElse(0, 0.0f);
    _size.y = _modelNode->ySize().getByFrameOrElse(0, 0.0f);
    _frameStart = static_cast<int>(_modelNode->frameStart().getByFrameOrElse(0, 0.0f));
    _frameEnd = static_cast<int>(_modelNode->frameEnd().getByFrameOrElse(0, 0.0f));
    _fps = _modelNode->fps().getByFrameOrElse(0, 0.0f);
    _spread = _modelNode->spread().getByFrameOrElse(0, 0.0f);
    _velocity = _modelNode->velocity().getByFrameOrElse(0, 0.0f);
    _randomVelocity = _modelNode->randVel().getByFrameOrElse(0, 0.0f);
    _mass = _modelNode->mass().getByFrameOrElse(0, 0.0f);
    _grav = _modelNode->grav().getByFrameOrElse(0, 0.0f);
    _lightningDelay = _modelNode->lightingDelay().getByFrameOrElse(0, 0.0f);
    _lightningRadius = _modelNode->lightingRadius().getByFrameOrElse(0, 0.0f);
    _lightningScale = _modelNode->lightingScale().getByFrameOrElse(0, 0.0f);
    _lightningSubDiv = static_cast<int>(_modelNode->lightingSubDiv().getByFrameOrElse(0, 0.0f));

    _particleSize.start = _modelNode->sizeStart().getByFrameOrElse(0, 0.0f);
    _particleSize.mid = _modelNode->sizeMid().getByFrameOrElse(0, 0.0f);
    _particleSize.end = _modelNode->sizeEnd().getByFrameOrElse(0, 0.0f);
    _color.start = _modelNode->colorStart().getByFrameOrElse(0, glm::vec3(0.0f));
    _color.mid = _modelNode->colorMid().getByFrameOrElse(0, glm::vec3(0.0f));
    _color.end = _modelNode->colorEnd().getByFrameOrElse(0, glm::vec3(0.0f));
    _alpha.start = _modelNode->alphaStart().getByFrameOrElse(0, 0.0f);
    _alpha.mid = _modelNode->alphaMid().getByFrameOrElse(0, 0.0f);
    _alpha.end = _modelNode->alphaEnd().getByFrameOrElse(0, 0.0f);

    if (_birthrate != 0.0f) {
        _birthInterval = 1.0f / _birthrate;
    }

    // Pre-allocate particles
    int numParticles;
    if (_modelNode->emitter()->updateMode == ModelNode::Emitter::UpdateMode::Single) {
        numParticles = 1;
    } else {
        numParticles = kMaxParticles;
    }
    for (int i = 0; i < numParticles; ++i) {
        _particlePool.push_back(newParticle());
    }
}

void EmitterSceneNode::update(float dt) {
    removeExpiredParticles(dt);
    spawnParticles(dt);

    for (auto &child : _children) {
        auto particle = static_pointer_cast<ParticleSceneNode>(child);
        particle->update(dt);
    }
}

void EmitterSceneNode::removeExpiredParticles(float dt) {
    if (_lifeExpectancy == -1.0f) {
        return;
    }
    vector<shared_ptr<SceneNode>> expiredParticles;
    for (auto &child : _children) {
        if (child->type() != SceneNodeType::Particle) {
            continue;
        }
        auto particle = static_pointer_cast<ParticleSceneNode>(child);
        if (particle->isExpired()) {
            expiredParticles.push_back(child);
        }
    }
    for (auto &particle : expiredParticles) {
        removeChild(particle);
        _particlePool.push_back(particle);
    }
}

void EmitterSceneNode::spawnParticles(float dt) {
    shared_ptr<ModelNode::Emitter> emitter(_modelNode->emitter());
    switch (emitter->updateMode) {
    case ModelNode::Emitter::UpdateMode::Fountain:
        if (_birthrate != 0.0f) {
            if (_birthTimer.advance(dt)) {
                doSpawnParticle();
                _birthTimer.setTimeout(_birthInterval);
            }
        }
        break;
    case ModelNode::Emitter::UpdateMode::Single:
        if (!_spawned || (_children.empty() && emitter->loop)) {
            doSpawnParticle();
            _spawned = true;
        }
        break;
    case ModelNode::Emitter::UpdateMode::Lightning:
        if (_birthTimer.advance(dt)) {
            spawnLightningParticles();
            _birthTimer.setTimeout(_lightningDelay);
        }
        break;
    default:
        break;
    }
}

void EmitterSceneNode::doSpawnParticle() {
    // Take particle from the pool, if available
    if (_particlePool.empty()) {
        return;
    }
    auto particle = static_pointer_cast<ParticleSceneNode>(_particlePool.front());
    particle->setLifetime(0.0f);

    float halfW = 0.005f * _size.x;
    float halfH = 0.005f * _size.y;
    glm::vec3 position(random(-halfW, halfW), random(-halfH, halfH), 0.0f);
    particle->setLocalTransform(glm::translate(position));

    float halfSpread = 0.5f * _spread;
    float angle1 = random(-halfSpread, halfSpread);
    float angle2 = random(-halfSpread, halfSpread);
    glm::vec3 dir(glm::sin(angle1), glm::sin(angle2), glm::cos(angle1) * glm::cos(angle2));
    glm::vec3 velocity((_velocity + random(0.0f, _randomVelocity)) * dir);
    particle->setVelocity(move(velocity));

    particle->setFrame(_frameStart);
    if (_fps > 0.0f) {
        particle->setAnimLength((_frameEnd - _frameStart + 1) / _fps);
    }

    // Remove particle from pool and append it to emitter
    _particlePool.pop_front();
    addChild(move(particle));
}

void EmitterSceneNode::spawnLightningParticles() {
    // Ensure there is a reference node directly under this emitter
    auto ref = find_if(_children.begin(), _children.end(), [](auto &child) { return child->type() == SceneNodeType::Dummy; });
    if (ref == _children.end()) {
        return;
    }

    float halfW = 0.005f * _size.x;
    float halfH = 0.005f * _size.y;
    glm::vec3 origin(random(-halfW, halfW), random(-halfH, halfH), 0.0f);
    glm::vec3 emitterSpaceRefPos(_absTransformInv * glm::vec4((*ref)->getOrigin(), 1.0f));
    glm::vec3 refToOrigin(emitterSpaceRefPos - origin);
    float distance = glm::abs(refToOrigin.z);
    float segmentLength = distance / static_cast<float>(_lightningSubDiv + 1);
    float halfRadius = 0.5f * _lightningRadius;

    vector<pair<glm::vec3, glm::vec3>> segments;
    segments.resize(_lightningSubDiv + 1);
    segments[0].first = origin;
    for (int i = 1; i < _lightningSubDiv + 1; ++i) {
        glm::vec3 dir(glm::normalize(emitterSpaceRefPos - segments[i - 1].first));
        glm::vec3 offset(
            random(-halfRadius, halfRadius),
            random(-halfRadius, halfRadius),
            0.0f);
        segments[i - 1].second = segments[i - 1].first + segmentLength * dir + offset;
        segments[i].first = segments[i - 1].second;
    }
    segments[_lightningSubDiv].second = emitterSpaceRefPos;

    // Return all particles to pool
    for (auto it = _children.begin(); it != _children.end();) {
        auto child = *it;
        if ((*it)->type() == SceneNodeType::Particle) {
            _particlePool.push_back(child);
            it = _children.erase(it);
        } else {
            ++it;
        }
    }

    for (auto &segment : segments) {
        // Take particle from the pool, if available
        if (_particlePool.empty()) {
            return;
        }
        auto particle = static_pointer_cast<ParticleSceneNode>(_particlePool.front());
        _particlePool.pop_front();
        particle->setLifetime(0.0f);

        glm::vec3 endToStart(segment.second - segment.first);
        glm::vec3 center(0.5f * (segment.first + segment.second));
        particle->setLocalTransform(glm::translate(center));
        particle->setDir(_absTransform * glm::vec4(glm::normalize(endToStart), 0.0f));
        particle->setSize(glm::vec2(_lightningScale, glm::length(endToStart)));

        addChild(move(particle));
    }
}

void EmitterSceneNode::detonate() {
    doSpawnParticle();
}

void EmitterSceneNode::drawLeafs(const vector<SceneNode *> &leafs) {
    if (leafs.empty()) {
        return;
    }
    auto emitter = _modelNode->emitter();
    auto texture = emitter->texture;
    if (!texture) {
        return;
    }
    auto emitterRight = glm::vec3(_absTransform[0]);
    auto emitterUp = glm::vec3(_absTransform[1]);
    auto emitterForward = glm::vec3(_absTransform[2]);

    auto view = _sceneGraph.activeCamera()->camera()->view();
    auto cameraRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
    auto cameraUp = glm::vec3(view[0][1], view[1][1], view[2][1]);
    auto cameraForward = glm::vec3(view[0][2], view[1][2], view[2][2]);

    auto &uniforms = _shaders.uniforms();
    uniforms.general.resetLocals();
    uniforms.general.featureMask = UniformsFeatureFlags::particles;
    if (emitter->blendMode == ModelNode::Emitter::BlendMode::PunchThrough) {
        uniforms.general.featureMask |= UniformsFeatureFlags::hashedalphatest;
    }
    uniforms.particles.gridSize = emitter->gridSize;

    for (size_t i = 0; i < leafs.size(); ++i) {
        auto particle = static_cast<ParticleSceneNode *>(leafs[i]);
        uniforms.particles.particles[i].positionFrame = glm::vec4(particle->getOrigin(), static_cast<float>(particle->frame()));
        uniforms.particles.particles[i].color = glm::vec4(particle->color(), particle->alpha());
        uniforms.particles.particles[i].size = glm::vec2(particle->size());
        switch (emitter->renderMode) {
        case ModelNode::Emitter::RenderMode::BillboardToLocalZ:
        case ModelNode::Emitter::RenderMode::MotionBlur:
            uniforms.particles.particles[i].right = glm::vec4(emitterUp, 0.0f);
            uniforms.particles.particles[i].up = glm::vec4(emitterRight, 0.0f);
            if (emitter->renderMode == ModelNode::Emitter::RenderMode::MotionBlur) {
                uniforms.particles.particles[i].size = glm::vec2(particle->size().x, (1.0f + kMotionBlurStrength * kProjectileSpeed) * particle->size().y);
            }
            break;
        case ModelNode::Emitter::RenderMode::BillboardToWorldZ:
            uniforms.particles.particles[i].right = glm::vec4(0.0f, 1.0f, 0.0, 0.0f);
            uniforms.particles.particles[i].up = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            break;
        case ModelNode::Emitter::RenderMode::AlignedToParticleDir:
            uniforms.particles.particles[i].right = glm::vec4(emitterRight, 0.0f);
            uniforms.particles.particles[i].up = glm::vec4(emitterForward, 0.0f);
            break;
        case ModelNode::Emitter::RenderMode::Linked: {
            auto particleUp = particle->dir();
            auto particleForward = glm::cross(particleUp, cameraRight);
            auto particleRight = glm::cross(particleForward, particleUp);
            uniforms.particles.particles[i].right = glm::vec4(particleRight, 0.0f);
            uniforms.particles.particles[i].up = glm::vec4(particleUp, 0.0f);
            break;
        }
        case ModelNode::Emitter::RenderMode::Normal:
        default:
            uniforms.particles.particles[i].right = glm::vec4(cameraRight, 0.0f);
            uniforms.particles.particles[i].up = glm::vec4(cameraUp, 0.0f);
            break;
        }
    }

    _shaders.use(_shaders.particle(), true);
    _textures.bind(*texture, TextureUnits::diffuseMap);
    BlendMode blendMode;
    switch (emitter->blendMode) {
    case ModelNode::Emitter::BlendMode::PunchThrough:
        blendMode = BlendMode::None;
        break;
    case ModelNode::Emitter::BlendMode::Lighten:
        blendMode = BlendMode::Lighten;
        break;
    case ModelNode::Emitter::BlendMode::Normal:
    default:
        blendMode = BlendMode::Normal;
        break;
    }
    _graphicsContext.withBlending(blendMode, [this, &leafs]() {
        _meshes.billboard().drawInstanced(leafs.size());
    });
}

unique_ptr<ParticleSceneNode> EmitterSceneNode::newParticle() {
    return make_unique<ParticleSceneNode>(*this, _sceneGraph);
}

} // namespace scene

} // namespace reone
