/*
 * Copyright (c) 2020 The reone project contributors
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

#include "placeable.h"

#include <boost/algorithm/string.hpp>

#include "../../common/streamutil.h"
#include "../../render/models.h"
#include "../../render/walkmeshes.h"
#include "../../resource/resources.h"
#include "../../scene/node/modelscenenode.h"

#include "../blueprint/blueprints.h"

#include "objectfactory.h"

using namespace std;

using namespace reone::render;
using namespace reone::resource;
using namespace reone::scene;

namespace reone {

namespace game {

Placeable::Placeable(uint32_t id, ObjectFactory *objectFactory, SceneGraph *sceneGraph) :
    SpatialObject(id, ObjectType::Placeable, sceneGraph),
    _objectFactory(objectFactory) {

    _drawDistance = 4096.0f;
}

void Placeable::load(const GffStruct &gffs) {
    _position[0] = gffs.getFloat("X");
    _position[1] = gffs.getFloat("Y");
    _position[2] = gffs.getFloat("Z");

    _heading = gffs.getFloat("Bearing");

    string templResRef(gffs.getString("TemplateResRef"));
    boost::to_lower(templResRef);

    loadBlueprint(templResRef);

    _conversation = _blueprint->conversation();

    updateTransform();
}

void Placeable::loadBlueprint(const string &resRef) {
    _blueprint = Blueprints::instance().getPlaceable(resRef);
    _tag = _blueprint->tag();
    _title = _blueprint->localizedName();

    shared_ptr<TwoDaTable> table(Resources::instance().get2DA("placeables"));

    string model(table->getString(_blueprint->appearance(), "modelname"));
    boost::to_lower(model);

    _model = make_unique<ModelSceneNode>(_sceneGraph, Models::instance().get(model));
    _model->setLightingEnabled(true);

    _walkmesh = Walkmeshes::instance().get(model, ResourceType::PlaceableWalkmesh);

    for (auto &itemResRef : _blueprint->items()) {
        shared_ptr<ItemBlueprint> itemBlueprint(Blueprints::instance().getItem(itemResRef));

        shared_ptr<Item> item(_objectFactory->newItem());
        item->load(itemBlueprint);

        _items.push_back(move(item));
    }

    _selectable = _blueprint->isUsable();
}

const PlaceableBlueprint &Placeable::blueprint() const {
    return *_blueprint;
}

} // namespace game

} // namespace reone
