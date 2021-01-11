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

#include <stdexcept>

#include "../common/log.h"
#include "../render/mesh/quad.h"
#include "../render/shaders.h"
#include "../render/textures.h"
#include "../render/util.h"
#include "../resource/resources.h"

using namespace std;
using namespace std::placeholders;

using namespace reone::render;
using namespace reone::resource;

namespace reone {

namespace gui {

GUI::GUI(GameVersion version, const GraphicsOptions &opts) : _version(version), _gfxOpts(opts) {
    _aspect = _gfxOpts.width / static_cast<float>(_gfxOpts.height);
    _screenCenter.x = _gfxOpts.width / 2;
    _screenCenter.y = _gfxOpts.height / 2;
}

string GUI::getResRef(const string &base) const {
    return _version == GameVersion::TheSithLords ? base + "_p" : base;
}

void GUI::load() {
    if (_resRef.empty()) {
        throw logic_error("resRef must not be empty");
    }
    info("GUI: load " + _resRef);

    shared_ptr<GffStruct> gui(Resources::instance().getGFF(_resRef, ResourceType::Gui));
    loadBackground(_backgroundType);

    ControlType type = Control::getType(*gui);
    string tag(Control::getTag(*gui));

    _rootControl = Control::of(this, type, tag);
    _rootControl->load(*gui);
    _controlByTag[tag] = _rootControl.get();

    switch (_scaling) {
        case ScalingMode::Center:
            _rootOffset.x = _screenCenter.x - _resolutionX / 2;
            _rootOffset.y = _screenCenter.y - _resolutionY / 2;
            break;
        case ScalingMode::Stretch:
            stretchControl(*_rootControl);
            break;
        default:
            break;
    }

    const Control::Extent &rootExtent = _rootControl->extent();
    _controlOffset = _rootOffset + glm::ivec2(rootExtent.left, rootExtent.top);

    for (auto &ctrlGffs : gui->getList("CONTROLS")) {
        loadControl(*ctrlGffs);
    }
}

void GUI::loadBackground(BackgroundType type) {
    string resRef;

    if ((_gfxOpts.width == 1600 && _gfxOpts.height == 1200) ||
        (_gfxOpts.width == 1280 && _gfxOpts.height == 960) ||
        (_gfxOpts.width == 1024 && _gfxOpts.height == 768) ||
        (_gfxOpts.width == 800 && _gfxOpts.height == 600)) {

        resRef = str(boost::format("%dx%d") % _gfxOpts.width % _gfxOpts.height);
    } else {
        resRef = "1600x1200";
    }
    switch (type) {
        case BackgroundType::Menu:
            resRef += "back";
            break;
        case BackgroundType::Load:
            resRef += "load";
            break;
        default:
            return;
    }
    _background = Textures::instance().get(resRef, TextureType::Diffuse);
}

void GUI::stretchControl(Control &control) {
    float aspectX = _gfxOpts.width / static_cast<float>(_resolutionX);
    float aspectY = _gfxOpts.height / static_cast<float>(_resolutionY);
    control.stretch(aspectX, aspectY);
}

void GUI::loadControl(const GffStruct &gffs) {
    ControlType type = Control::getType(gffs);
    string tag(Control::getTag(gffs));
    string parent(Control::getParent(gffs));

    unique_ptr<Control> control(Control::of(this, type, tag));
    if (!control) return;

    preloadControl(*control);
    control->load(gffs);
    if (_hasDefaultHilightColor) {
        control->setHilightColor(_defaultHilightColor);
    }

    ScalingMode scaling = _scaling;
    auto maybeScaling = _scalingByControlTag.find(tag);
    if (maybeScaling != _scalingByControlTag.end()) {
        scaling = maybeScaling->second;
    }
    switch (scaling) {
        case ScalingMode::PositionRelativeToCenter:
            positionRelativeToCenter(*control);
            break;
        case ScalingMode::Stretch:
            stretchControl(*control);
            break;
        default:
            break;
    }

    _controlByTag[tag] = control.get();
    _controls.push_back(move(control));
}

void GUI::preloadControl(Control &control) {
}

void GUI::configureRootContol(const function<void(Control &)> &fn) {
    fn(*_rootControl);
}

void GUI::configureControl(const string &tag, const function<void(Control &)> &fn) {
    auto maybeControl = _controlByTag.find(tag);
    if (maybeControl != _controlByTag.end()) {
        fn(*maybeControl->second);
    }
}

void GUI::positionRelativeToCenter(Control &control) {
    Control::Extent extent(control.extent());
    if (extent.left >= 0.5f * _resolutionX) {
        extent.left = extent.left - _resolutionX + _gfxOpts.width;
    }
    if (extent.top >= 0.5f * _resolutionY) {
        extent.top = extent.top - _resolutionY + _gfxOpts.height;
    }
    control.setExtent(move(extent));
}

bool GUI::handle(const SDL_Event &event) {
    switch (event.type) {
        case SDL_KEYDOWN:
            return handleKeyDown(event.key.keysym.scancode);

        case SDL_KEYUP:
            return handleKeyUp(event.key.keysym.scancode);

        case SDL_MOUSEMOTION: {
            glm::ivec2 ctrlCoords(event.motion.x - _controlOffset.x, event.motion.y - _controlOffset.y);
            updateFocus(ctrlCoords.x, ctrlCoords.y);
            if (_focus) {
                _focus->handleMouseMotion(ctrlCoords.x, ctrlCoords.y);
            }
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            if (event.button.button == SDL_BUTTON_LEFT) {
                glm::ivec2 ctrlCoords(event.button.x - _controlOffset.x, event.button.y - _controlOffset.y);
                Control *control = getControlAt(ctrlCoords.x, ctrlCoords.y, [](const Control &ctrl) { return ctrl.isClickable(); });
                if (control) {
                    debug("GUI: click: " + control->tag());
                    return control->handleClick(ctrlCoords.x, ctrlCoords.y);
                }
            }
            break;
        }

        case SDL_MOUSEWHEEL:
            if (_focus && _focus->handleMouseWheel(event.wheel.x, event.wheel.y)) return true;
            break;
    }

    return false;
}

bool GUI::handleKeyDown(SDL_Scancode key) {
    return false;
}

bool GUI::handleKeyUp(SDL_Scancode key) {
    return false;
}

void GUI::updateFocus(int x, int y) {
    resetFocus();

    Control *control = getControlAt(x, y, [](const Control &ctrl) { return ctrl.isFocusable(); });
    if (control) {
        control->setFocus(true);
        _focus = control;
        onFocusChanged(_focus->tag(), true);
    }
}

Control *GUI::getControlAt(int x, int y, const function<bool(const Control &)> &test) const {
    for (auto it = _controls.rbegin(); it != _controls.rend(); ++it) {
        Control *ctrl = (*it).get();
        if (!ctrl->isVisible() || ctrl->isDisabled() || !test(*ctrl)) continue;

        if (ctrl->extent().contains(x, y)) {
            return ctrl;
        }
    }

    return nullptr;
}

void GUI::onFocusChanged(const string &control, bool focus) {
}

void GUI::onListBoxItemClick(const string &control, const std::string &item) {
}

void GUI::update(float dt) {
    for (auto &control : _controls) {
        control->update(dt);
    }
}

void GUI::render() const {
    if (_background) drawBackground();
    if (_rootControl) _rootControl->render(_rootOffset);

    for (auto &control : _controls) {
        control->render(_controlOffset);
    }
}

void GUI::render3D() const {
    for (auto &control : _controls) {
        control->render3D(_controlOffset);
    }
}

void GUI::drawBackground() const {
    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, glm::vec3(0.0f, 0.0f, -0.9));
    transform = glm::scale(transform, glm::vec3(_gfxOpts.width, _gfxOpts.height, 1.0f));

    LocalUniforms locals;
    locals.general.model = move(transform);

    Shaders::instance().activate(ShaderProgram::GUIGUI, locals);

    setActiveTextureUnit(0);
    _background->bind();

    Quad::getDefault().renderTriangles();
}

void GUI::resetFocus() {
    if (_focus) {
        if (_focus->isFocusable()) {
            _focus->setFocus(false);
        }
        onFocusChanged(_focus->tag(), false);
        _focus = nullptr;
    }
}

void GUI::showControl(const string &tag) {
    configureControl(tag, [](Control &ctrl) { ctrl.setVisible(true); });
}

void GUI::hideControl(const string &tag) {
    configureControl(tag, [](Control &ctrl) { ctrl.setVisible(false); });
}

void GUI::enableControl(const string &tag) {
    configureControl(tag, [](Control &ctrl) { ctrl.setDisabled(false); });
}

void GUI::disableControl(const string &tag) {
    configureControl(tag, [](Control &ctrl) { ctrl.setDisabled(true); });
}

void GUI::setControlFocusable(const string &tag, bool focusable) {
    configureControl(tag, [&focusable](Control &ctrl) { ctrl.setFocusable(focusable); });
}

void GUI::setControlDisabled(const string &tag, bool disabled) {
    configureControl(tag, [&disabled](Control &ctrl) { ctrl.setDisabled(disabled); });
}

void GUI::setControlText(const string &tag, const string &text) {
    configureControl(tag, [&text](Control &ctrl) { ctrl.setTextMessage(text); });
}

void GUI::setControlFocus(const string &tag, bool focus) {
    configureControl(tag, [&focus](Control &ctrl) { ctrl.setFocus(focus); });
}

void GUI::setControlVisible(const string &tag, bool visible) {
    configureControl(tag, [&visible](Control &ctrl) { ctrl.setVisible(visible); });
}

void GUI::setControlDiscardColor(const string &tag, glm::vec3 color) {
    configureControl(tag, [&color](Control & ctrl) { ctrl.setDiscardColor(color); });
}

Control &GUI::getControl(const string &tag) const {
    auto it = find_if(
        _controls.begin(),
        _controls.end(),
        [&tag](const unique_ptr<Control> &ctrl) { return ctrl->tag() == tag; });

    return **it;
}

void GUI::onClick(const string &control) {
}

} // namespace gui

} // namespace reone
