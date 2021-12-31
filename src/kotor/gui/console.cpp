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

#include "console.h"

#include "../../common/logutil.h"
#include "../../game/effect/factory.h"
#include "../../game/game.h"
#include "../../game/object/creature.h"
#include "../../game/party.h"
#include "../../game/services.h"
#include "../../graphics/context.h"
#include "../../graphics/font.h"
#include "../../graphics/fonts.h"
#include "../../graphics/mesh.h"
#include "../../graphics/meshes.h"
#include "../../graphics/shaders.h"
#include "../../graphics/window.h"
#include "../../resource/resources.h"
#include "../../scene/types.h"

using namespace std;
using namespace std::placeholders;

using namespace reone::game;
using namespace reone::gui;
using namespace reone::graphics;
using namespace reone::scene;

namespace reone {

namespace kotor {

static constexpr int kMaxOutputLineCount = 100;
static constexpr int kVisibleLineCount = 15;

Console::Console(
    Game &game,
    Services &services) :
    _game(game),
    _services(services),
    _input(TextInputFlags::console) {

    initCommands();
}

void Console::initCommands() {
    addCommand("clear", bind(&Console::cmdClear, this, _1));
    addCommand("describe", bind(&Console::cmdDescribe, this, _1));
    addCommand("listanim", bind(&Console::cmdListAnim, this, _1));
    addCommand("playanim", bind(&Console::cmdPlayAnim, this, _1));
    addCommand("kill", bind(&Console::cmdKill, this, _1));
    addCommand("additem", bind(&Console::cmdAddItem, this, _1));
    addCommand("givexp", bind(&Console::cmdGiveXP, this, _1));
    addCommand("warp", bind(&Console::cmdWarp, this, _1));
}

void Console::addCommand(const std::string &name, const CommandHandler &handler) {
    _commands.insert(make_pair(name, handler));
}

void Console::cmdClear(vector<string> tokens) {
    _output.clear();
    _outputOffset = 0;
}

void Console::cmdDescribe(vector<string> tokens) {
    auto object = _game.module()->area()->selectedObject();
    if (!object) {
        print("describe: no object selected");
        return;
    }
    glm::vec3 position(object->position());

    stringstream ss;
    ss
        << setprecision(2) << fixed
        << "id=" << object->id()
        << " "
        << "tag=\"" << object->tag() << "\""
        << " "
        << "tpl=\"" << object->blueprintResRef() << "\""
        << " "
        << "pos=[" << position.x << ", " << position.y << ", " << position.z << "]";

    switch (object->type()) {
    case ObjectType::Creature: {
        auto creature = static_pointer_cast<Creature>(object);
        ss << " "
           << "fac=" << static_cast<int>(creature->faction());
        break;
    }
    default:
        break;
    }

    print(ss.str());
}

void Console::cmdListAnim(vector<string> tokens) {
    ;
    auto object = _game.module()->area()->selectedObject();
    if (!object) {
        object = _game.party().getLeader();
        if (!object) {
            print("listanim: no object selected");
            return;
        }
    }

    string substr;
    if (static_cast<int>(tokens.size()) > 1) {
        substr = tokens[1];
    }

    auto model = static_pointer_cast<ModelSceneNode>(object->sceneNode());
    vector<string> anims(model->model().getAnimationNames());
    sort(anims.begin(), anims.end());

    for (auto &anim : anims) {
        if (substr.empty() || boost::contains(anim, substr)) {
            print(anim);
        }
    }
}

void Console::cmdPlayAnim(vector<string> tokens) {
    if (tokens.size() < 2) {
        print("Usage: playanim anim_name");
        return;
    }
    auto object = _game.module()->area()->selectedObject();
    if (!object) {
        object = _game.party().getLeader();
        if (!object) {
            print("playanim: no object selected");
            return;
        }
    }
    auto model = static_pointer_cast<ModelSceneNode>(object->sceneNode());
    model->playAnimation(tokens[1], AnimationProperties::fromFlags(AnimationFlags::loop));
}

void Console::cmdKill(vector<string> tokens) {
    auto object = _game.module()->area()->selectedObject();
    if (!object) {
        print("kill: no object selected");
        return;
    }
    auto effect = _game.effectFactory().newDamage(100000, DamageType::Universal, nullptr);
    object->applyEffect(move(effect), DurationType::Instant);
}

void Console::cmdAddItem(vector<string> tokens) {
    if (tokens.size() < 2) {
        print("Usage: additem item_tpl [size]");
        return;
    }
    auto object = _game.module()->area()->selectedObject();
    if (!object) {
        object = _game.party().getLeader();
        if (!object) {
            print("additem: no object selected");
            return;
        }
    }
    int stackSize = static_cast<int>(tokens.size()) > 2 ? stoi(tokens[2]) : 1;
    object->addItem(tokens[1], stackSize);
}

void Console::cmdGiveXP(vector<string> tokens) {
    if (tokens.size() < 2) {
        print("Usage: givexp amount");
        return;
    }

    auto object = _game.module()->area()->selectedObject();
    if (!object) {
        object = _game.party().getLeader();
    }
    if (!object || object->type() != ObjectType::Creature) {
        print("givexp: no creature selected");
        return;
    }

    int amount = stoi(tokens[1]);
    static_pointer_cast<Creature>(object)->giveXP(amount);
}

void Console::cmdWarp(vector<string> tokens) {
    if (tokens.size() < 2) {
        print("Usage: warp module");
        return;
    }
    _game.loadModule(tokens[1]);
}

void Console::print(const string &text) {
    _output.push_front(text);
    trimOutput();
}

void Console::trimOutput() {
    for (int i = static_cast<int>(_output.size()) - kMaxOutputLineCount; i > 0; --i) {
        _output.pop_back();
    }
}

void Console::init() {
    _font = _services.fonts.get("fnt_console");
}

bool Console::handle(const SDL_Event &event) {
    if (_open && _input.handle(event))
        return true;

    switch (event.type) {
    case SDL_MOUSEWHEEL:
        return handleMouseWheel(event.wheel);
    case SDL_KEYUP:
        return handleKeyUp(event.key);
    default:
        return false;
    }
}

bool Console::handleMouseWheel(const SDL_MouseWheelEvent &event) {
    bool up = event.y < 0;
    if (up) {
        if (_outputOffset > 0) {
            --_outputOffset;
        }
    } else {
        if (_outputOffset < static_cast<int>(_output.size()) - kVisibleLineCount + 1) {
            ++_outputOffset;
        }
    }
    return true;
}

bool Console::handleKeyUp(const SDL_KeyboardEvent &event) {
    if (_open) {
        switch (event.keysym.sym) {
        case SDLK_BACKQUOTE:
            _open = false;
            return true;

        case SDLK_RETURN: {
            string text(_input.text());
            if (!text.empty()) {
                executeInputText();
                _history.push(_input.text());
                _input.clear();
            }
            return true;
        }
        case SDLK_UP:
            if (!_history.empty()) {
                _input.setText(_history.top());
                _history.pop();
            }
            return true;
        default:
            return false;
        }
    } else {
        switch (event.keysym.sym) {
        case SDLK_BACKQUOTE:
            _open = true;
            return true;

        default:
            return false;
        }
    }
}

void Console::executeInputText() {
    vector<string> tokens;
    boost::split(tokens, _input.text(), boost::is_space(), boost::token_compress_on);

    if (tokens.empty())
        return;

    auto maybeCommand = _commands.find(tokens[0]);
    if (maybeCommand != _commands.end()) {
        maybeCommand->second(move(tokens));
    } else {
        print("Unsupported command: " + tokens[0]);
    }
}

void Console::draw() {
    _services.graphicsContext.withBlending(BlendMode::Normal, [this]() {
        drawBackground();
        drawLines();
    });
}

void Console::drawBackground() {
    float height = kVisibleLineCount * _font->height();

    glm::mat4 transform(1.0f);
    transform = glm::scale(transform, glm::vec3(_game.options().graphics.width, height, 1.0f));

    auto &uniforms = _services.shaders.uniforms();
    uniforms.general.resetLocals();
    uniforms.general.projection = _services.window.getOrthoProjection();
    uniforms.general.model = move(transform);
    uniforms.general.color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    uniforms.general.alpha = 0.5f;

    _services.shaders.use(_services.shaders.simpleColor(), true);
    _services.meshes.quad().draw();
}

void Console::drawLines() {
    float height = kVisibleLineCount * _font->height();

    glm::vec3 position(3.0f, height - 0.5f * _font->height(), 0.0f);

    // Input

    string text("> " + _input.text());
    _font->draw(text, position, glm::vec3(1.0f), TextGravity::RightCenter);

    // Output

    for (int i = 0; i < kVisibleLineCount - 1 && i < static_cast<int>(_output.size()) - _outputOffset; ++i) {
        const string &line = _output[static_cast<size_t>(i) + _outputOffset];
        position.y -= _font->height();
        _font->draw(line, position, glm::vec3(1.0f), TextGravity::RightCenter);
    }
}

} // namespace kotor

} // namespace reone
