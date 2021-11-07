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

#include "../../../resource/format/binreader.h"

#include "../modelnode.h"

namespace reone {

namespace graphics {

class Animation;
class Model;
class Models;
class Textures;

class MdlReader : public resource::BinaryReader {
public:
    MdlReader(Models &models, Textures &textures);

    void load(const std::shared_ptr<std::istream> &mdl, const std::shared_ptr<std::istream> &mdx);

    std::shared_ptr<graphics::Model> model() const { return _model; }

private:
    struct ArrayDefinition {
        uint32_t offset {0};
        uint32_t count {0};
        uint32_t count2 {0};
    };

    struct ControllerKey {
        uint32_t type {0};
        uint16_t numRows {0};
        uint16_t timeIndex {0};
        uint16_t dataIndex {0};
        uint8_t numColumns {0};
    };

    typedef std::unordered_map<uint32_t, std::vector<uint32_t>> MaterialMap;
    typedef std::function<void(const ControllerKey &, const std::vector<float> &, ModelNode &)> ControllerFn;

    Models &_models;
    Textures &_textures;

    std::unordered_map<uint32_t, ControllerFn> _genericControllers;
    std::unordered_map<uint32_t, ControllerFn> _meshControllers;
    std::unordered_map<uint32_t, ControllerFn> _lightControllers;
    std::unordered_map<uint32_t, ControllerFn> _emitterControllers;

    std::unique_ptr<StreamReader> _mdxReader;
    bool _tsl {false}; /**< is this a TSL model? */
    std::vector<std::string> _nodeNames;
    std::vector<std::shared_ptr<ModelNode>> _nodes; /**< loaded model nodes (DFS ordering) */
    std::map<uint16_t, uint16_t> _nodeFlags;
    std::shared_ptr<graphics::Model> _model;

    void doLoad() override;

    ArrayDefinition readArrayDefinition();
    void readNodeNames(const std::vector<uint32_t> &offsets);
    std::shared_ptr<graphics::ModelNode> readNode(uint32_t offset, const ModelNode *parent, bool anim = false);
    std::vector<std::shared_ptr<graphics::Animation>> readAnimations(const std::vector<uint32_t> &offsets);
    std::unique_ptr<graphics::Animation> readAnimation(uint32_t offset);
    void readControllers(uint32_t keyOffset, uint32_t keyCount, const std::vector<float> &data, bool anim, graphics::ModelNode &node);

    std::shared_ptr<ModelNode::Reference> readReference();
    std::shared_ptr<ModelNode::Light> readLight();
    std::shared_ptr<ModelNode::Emitter> readEmitter();
    std::shared_ptr<ModelNode::TriangleMesh> readMesh(int flags);

    std::shared_ptr<ModelNode::AABBTree> readAABBTree(uint32_t offset);

    void prepareSkinMeshes();

    // Controllers

    void initControllerFn();

    ControllerFn getControllerFn(uint32_t type, int nodeFlags);

    static void readPositionController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readOrientationController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readScaleController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readAlphaController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readSelfIllumColorController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readColorController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readRadiusController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readShadowRadiusController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readVerticalDisplacementController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readMultiplierController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readAlphaEndController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readAlphaStartController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readBirthrateController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readBounceCoController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readCombineTimeController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readDragController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readFPSController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readFrameEndController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readFrameStartController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readGravController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readLifeExpController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readMassController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readP2PBezier2Controller(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readP2PBezier3Controller(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readParticleRotController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readRandVelController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readSizeStartController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readSizeEndController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readSizeStartYController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readSizeEndYController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readSpreadController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readThresholdController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readVelocityController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readXSizeController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readYSizeController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readBlurLengthController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readLightingDelayController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readLightingRadiusController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readLightingScaleController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readLightingSubDivController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readLightingZigZagController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readAlphaMidController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readPercentStartController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readPercentMidController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readPercentEndController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readSizeMidController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readSizeMidYController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readRandomBirthRateController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readTargetSizeController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readNumControlPtsController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readControlPtRadiusController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readControlPtDelayController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readTangentSpreadController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readTangentLengthController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readColorMidController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readColorEndController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readColorStartController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);
    static void readDetonateController(const ControllerKey &key, const std::vector<float> &data, ModelNode &node);

    static void readFloatController(const ControllerKey &key, const std::vector<float> &data, AnimatedProperty<float> &prop);
    static void readVectorController(const ControllerKey &key, const std::vector<float> &data, AnimatedProperty<glm::vec3> &prop);

    // END Controllers
};

} // namespace graphics

} // namespace reone
