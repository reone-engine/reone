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

#include "creaturemodelbuilder.h"

#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "../../common/streamutil.h"
#include "../../render/model/models.h"
#include "../../render/textures.h"
#include "../../resource/resources.h"
#include "../../scene/types.h"

#include "creature.h"

using namespace std;

using namespace reone::render;
using namespace reone::resource;
using namespace reone::scene;

namespace reone {

namespace game {

// This is a hook to replace MDL models with GR2 models (SWTOR).
static const unordered_map<string, string> g_gr2Models {
    //{ "c_rancors", "rancor_rancor_a01" }
};

static const string g_headHookNode("headhook");
static const string g_maskHookNode("gogglehook");

CreatureModelBuilder::CreatureModelBuilder(Creature *creature) : _creature(creature) {
    if (!creature) {
        throw invalid_argument("creature must not be null");
    }
}

shared_ptr<ModelSceneNode> CreatureModelBuilder::build() {
    string modelName(getBodyModelName());
    if (modelName.empty()) return nullptr;

    bool gr2Model = false;
    auto maybeGr2Model = g_gr2Models.find(modelName);
    if (maybeGr2Model != g_gr2Models.end()) {
        modelName = maybeGr2Model->second;
        gr2Model = true;
    }

    shared_ptr<Model> model(Models::instance().get(modelName, gr2Model ? ResourceType::Gr2 : ResourceType::Mdl));
    if (!model) return nullptr;

    auto modelSceneNode = make_unique<ModelSceneNode>(ModelSceneNode::Classification::Creature, model, &_creature->sceneGraph());
    if (gr2Model) return move(modelSceneNode);

    // Body texture

    string bodyTextureName(getBodyTextureName());
    if (!bodyTextureName.empty()) {
        shared_ptr<Texture> texture(Textures::instance().get(bodyTextureName, TextureUsage::Diffuse));
        modelSceneNode->setDiffuseTexture(texture);
    }

    // Mask

    shared_ptr<Model> maskModel;
    string maskModelName(getMaskModelName());
    if (!maskModelName.empty()) {
        maskModel = Models::instance().get(maskModelName);
    }

    // Head

    string headModelName(getHeadModelName());
    if (!headModelName.empty()) {
        shared_ptr<Model> headModel(Models::instance().get(headModelName));
        if (headModel) {
            shared_ptr<ModelSceneNode> headSceneNode(modelSceneNode->attach(g_headHookNode, headModel, ModelSceneNode::Classification::Creature));
            if (headSceneNode && maskModel) {
                headSceneNode->attach(g_maskHookNode, maskModel, ModelSceneNode::Classification::Equipment);
            }
        }
    }

    // Left weapon

    string leftWeaponModelName(getWeaponModelName(InventorySlot::leftWeapon));
    if (!leftWeaponModelName.empty()) {
        shared_ptr<Model> leftWeaponModel(Models::instance().get(leftWeaponModelName));
        if (leftWeaponModel) {
            modelSceneNode->attach("lhand", leftWeaponModel, ModelSceneNode::Classification::Equipment);
        }
    }

    // Right weapon

    string rightWeaponModelName(getWeaponModelName(InventorySlot::rightWeapon));
    if (!rightWeaponModelName.empty()) {
        shared_ptr<Model> rightWeaponModel(Models::instance().get(rightWeaponModelName));
        if (rightWeaponModel) {
            modelSceneNode->attach("rhand", rightWeaponModel, ModelSceneNode::Classification::Equipment);
        }
    }

    return move(modelSceneNode);
}

string CreatureModelBuilder::getBodyModelName() const {
    string column;

    if (_creature->modelType() == Creature::ModelType::Character) {
        column = "model";

        shared_ptr<Item> bodyItem(_creature->getEquippedItem(InventorySlot::body));
        if (bodyItem) {
            string baseBodyVar(bodyItem->baseBodyVariation());
            column += baseBodyVar;
        } else {
            column += "a";
        }

    } else {
        column = "race";
    }

    shared_ptr<TwoDA> appearance(Resources::instance().get2DA("appearance"));

    string modelName(appearance->getString(_creature->appearance(), column));
    boost::to_lower(modelName);

    return move(modelName);
}

string CreatureModelBuilder::getBodyTextureName() const {
    string column;
    shared_ptr<Item> bodyItem(_creature->getEquippedItem(InventorySlot::body));

    if (_creature->modelType() == Creature::ModelType::Character) {
        column = "tex";

        if (bodyItem) {
            string baseBodyVar(bodyItem->baseBodyVariation());
            column += baseBodyVar;
        } else {
            column += "a";
        }
    } else {
        column = "racetex";
    }

    shared_ptr<TwoDA> appearance(Resources::instance().get2DA("appearance"));

    string texName(appearance->getString(_creature->appearance(), column));
    boost::to_lower(texName);

    if (texName.empty()) return "";

    if (_creature->modelType() == Creature::ModelType::Character) {
        if (bodyItem) {
            texName += str(boost::format("%02d") % bodyItem->textureVariation());
        } else {
            texName += "01";
        }
    }

    return move(texName);
}

string CreatureModelBuilder::getHeadModelName() const {
    if (_creature->modelType() != Creature::ModelType::Character) return "";

    shared_ptr<TwoDA> appearance(Resources::instance().get2DA("appearance"));

    int headIdx = appearance->getInt(_creature->appearance(), "normalhead", -1);
    if (headIdx == -1) return "";

    shared_ptr<TwoDA> heads(Resources::instance().get2DA("heads"));

    string modelName(heads->getString(headIdx, "head"));
    boost::to_lower(modelName);

    return move(modelName);
}

string CreatureModelBuilder::getMaskModelName() const {
    shared_ptr<Item> headItem(_creature->getEquippedItem(InventorySlot::head));
    if (!headItem) return "";

    string modelName(boost::to_lower_copy(headItem->itemClass()));
    modelName += str(boost::format("_%03d") % headItem->modelVariation());

    return move(modelName);

}

string CreatureModelBuilder::getWeaponModelName(int slot) const {
    shared_ptr<Item> bodyItem(_creature->getEquippedItem(slot));
    if (!bodyItem) return "";

    string modelName(bodyItem->itemClass());
    boost::to_lower(modelName);

    modelName += str(boost::format("_%03d") % bodyItem->modelVariation());

    return move(modelName);
}

} // namespace game

} // namespace reone
