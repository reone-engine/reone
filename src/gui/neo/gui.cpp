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

#include "gui.h"

#include "../../common/exception/validation.h"
#include "../../common/logutil.h"
#include "../../graphics/options.h"
#include "../../graphics/services.h"
#include "../../graphics/uniforms.h"
#include "../../resource/gffs.h"
#include "../../resource/services.h"

#include "../types.h"

using namespace std;

using namespace reone::resource;

namespace reone {

namespace gui {

namespace neo {

void Gui::load(const string &resRef) {
    info("Loading GUI " + resRef, LogChannels::gui);

    auto gui = _resourceSvc.gffs.get(resRef, ResourceType::Gui);
    if (!gui) {
        throw ValidationException("GUI not found: " + resRef);
    }

    _rootControl = loadControl(*gui);

    auto controls = gui->getList("CONTROLS");
    for (auto &control : controls) {
        _rootControl->append(loadControl(*control));
    }
}

unique_ptr<Control> Gui::loadControl(const Gff &gui) {
    auto id = gui.getInt("ID", -1);
    auto tag = gui.getString("TAG");
    auto type = static_cast<ControlType>(gui.getInt("CONTROLTYPE"));

    unique_ptr<Control> control;
    if (type == ControlType::Panel) {
        control = newPanel(id, move(tag));
    } else if (type == ControlType::Label) {
        control = newLabel(id, move(tag));
    } else if (type == ControlType::LabelHilight) {
        control = newLabelHilight(id, move(tag));
    } else if (type == ControlType::ProgressBar) {
        control = newProgressBar(id, move(tag));
    } else {
        throw ValidationException("Unsupported control type: " + to_string(static_cast<int>(type)));
    }
    control->load(gui);

    return move(control);
}

bool Gui::handle(const SDL_Event &e) {
    return false;
}

void Gui::update(float delta) {
}

void Gui::render() {
    _graphicsSvc.uniforms.setGeneral([this](auto &u) {
        u.resetGlobals();
        u.projection = glm::ortho(
            0.0f,
            static_cast<float>(_graphicsOpt.width),
            static_cast<float>(_graphicsOpt.height),
            0.0f);
    });
    _rootControl->render();
}

unique_ptr<Label> Gui::newLabel(int id, string tag) {
    return make_unique<Label>(id, move(tag), _graphicsOpt, _graphicsSvc);
}

unique_ptr<LabelHilight> Gui::newLabelHilight(int id, string tag) {
    return make_unique<LabelHilight>(id, move(tag), _graphicsOpt, _graphicsSvc);
}

unique_ptr<Panel> Gui::newPanel(int id, string tag) {
    return make_unique<Panel>(id, move(tag), _graphicsOpt, _graphicsSvc);
}

unique_ptr<ProgressBar> Gui::newProgressBar(int id, string tag) {
    return make_unique<ProgressBar>(id, move(tag), _graphicsOpt, _graphicsSvc);
}

} // namespace neo

} // namespace gui

} // namespace reone