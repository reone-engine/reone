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

#include "model.h"

#include <stdexcept>

#include "../../common/log.h"

using namespace std;

namespace reone {

namespace render {

Model::Model(
    const string &name,
    Classification classification,
    const shared_ptr<ModelNode> &rootNode,
    vector<shared_ptr<Animation>> &anims,
    const shared_ptr<Model> &superModel
) :
    _name(name),
    _classification(classification),
    _rootNode(rootNode),
    _superModel(superModel) {

    if (!rootNode) {
        throw invalid_argument("rootNode must not be null");
    }
    for (auto &anim : anims) {
        _animations.insert(make_pair(anim->name(), move(anim)));
    }

    init(_rootNode);
}

void Model::init(const shared_ptr<ModelNode> &node) {
    _nodeByNumber.insert(make_pair(node->nodeNumber(), node));
    _nodeByName.insert(make_pair(node->name(), node));
    _maxNodeIndex = glm::max(_maxNodeIndex, node->index());

    shared_ptr<ModelMesh> mesh(node->mesh());
    if (mesh) {
        _aabb.expand(mesh->mesh()->aabb() * node->absoluteTransform());
    }

    for (auto &child : node->children()) {
        init(child);
    }
}

void Model::initGL() {
    _rootNode->initGL();
}

vector<string> Model::getAnimationNames() const {
    vector<string> result;

    if (_superModel) {
        vector<string> superAnims(_superModel->getAnimationNames());
        for (auto &anim : superAnims) {
            result.push_back(anim);
        }
    }
    for (auto &anim : _animations) {
        result.push_back(anim.first);
    }

    return move(result);
}

shared_ptr<Animation> Model::getAnimation(const string &name) const {
    auto maybeAnim = _animations.find(name);
    if (maybeAnim != _animations.end()) {
        return maybeAnim->second;
    }
    if (_superModel) {
        return _superModel->getAnimation(name);
    }
    debug(boost::format("Model: animation not found: '%s' '%s'") % name % _name, 2);

    return nullptr;
}

shared_ptr<ModelNode> Model::findNodeByNumber(uint16_t number) const {
    auto it = _nodeByNumber.find(number);
    return it != _nodeByNumber.end() ? it->second : nullptr;
}

shared_ptr<ModelNode> Model::findNodeByName(const string &name) const {
    auto it = _nodeByName.find(name);
    return it != _nodeByName.end() ? it->second : nullptr;
}

void Model::setAnimationScale(float scale) {
    _animationScale = scale;
}

} // namespace render

} // namespace reone
