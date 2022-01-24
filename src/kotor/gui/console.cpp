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
#include "../../game/debug.h"
#include "../../game/effect/factory.h"
#include "../../game/game.h"
#include "../../game/location.h"
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
#include "../../script/executioncontext.h"
#include "../../script/routine.h"
#include "../../script/routines.h"
#include "../../script/variable.h"

using namespace std;
using namespace std::placeholders;

using namespace reone::game;
using namespace reone::gui;
using namespace reone::graphics;
using namespace reone::scene;
using namespace reone::script;

namespace reone {

namespace kotor {

static constexpr int kMaxOutputLineCount = 100;
static constexpr int kVisibleLineCount = 15;

void Console::init() {
    _font = _services.fonts.get("fnt_console");

    addCommand("clear", "c", "clear console", bind(&Console::cmdClear, this, _1, _2));
    addCommand("info", "i", "information on selected object", bind(&Console::cmdInfo, this, _1, _2));
    addCommand("listglobals", "lg", "list global variables", bind(&Console::cmdListGlobals, this, _1, _2));
    addCommand("listlocals", "ll", "list local variables", bind(&Console::cmdListLocals, this, _1, _2));
    addCommand("listroutine", "lr", "list script routines", bind(&Console::cmdListRoutine, this, _1, _2));
    addCommand("exec", "x", "execute script routine", bind(&Console::cmdExec, this, _1, _2));
    addCommand("listanim", "la", "list animations of selected object", bind(&Console::cmdListAnim, this, _1, _2));
    addCommand("playanim", "pa", "play animation on selected object", bind(&Console::cmdPlayAnim, this, _1, _2));
    addCommand("warp", "w", "warp to a module", bind(&Console::cmdWarp, this, _1, _2));
    addCommand("kill", "k", "kill selected object", bind(&Console::cmdKill, this, _1, _2));
    addCommand("additem", "ai", "add item to selected object", bind(&Console::cmdAddItem, this, _1, _2));
    addCommand("givexp", "xp", "give experience to selected creature", bind(&Console::cmdGiveXP, this, _1, _2));
    addCommand("showwalkmesh", "sw", "toggle rendering walkmesh", bind(&Console::cmdShowWalkmesh, this, _1, _2));
    addCommand("showtriggers", "st", "toggle rendering triggers", bind(&Console::cmdShowTriggers, this, _1, _2));

    addCommand("help", "h", "list console commands", bind(&Console::cmdHelp, this, _1, _2));
}

void Console::addCommand(string name, string alias, string description, CommandHandler handler) {
    Command cmd;
    cmd.name = move(name);
    cmd.alias = move(alias);
    cmd.description = move(description);
    cmd.handler = move(handler);

    _commands.push_back(cmd);
    _commandByName.insert(make_pair(cmd.name, cmd));

    if (!cmd.alias.empty()) {
        _commandByAlias.insert(make_pair(cmd.alias, cmd));
    }
}

bool Console::handle(const SDL_Event &event) {
    if (_open && _input.handle(event)) {
        return true;
    }
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
    if (tokens.empty()) {
        return;
    }
    CommandHandler handler;
    auto maybeCmdByName = _commandByName.find(tokens[0]);
    if (maybeCmdByName != _commandByName.end()) {
        handler = maybeCmdByName->second.handler;
    } else {
        auto maybeCmdByAlias = _commandByAlias.find(tokens[0]);
        if (maybeCmdByAlias != _commandByAlias.end()) {
            handler = maybeCmdByAlias->second.handler;
        }
    }
    if (handler) {
        try {
            handler(_input.text(), move(tokens));
        } catch (const invalid_argument &) {
            print("Invalid argument");
        }
    } else {
        print("Unknown command");
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

void Console::cmdClear(string input, vector<string> tokens) {
    _output.clear();
    _outputOffset = 0;
}

void Console::cmdInfo(string input, vector<string> tokens) {
    auto object = _game.module()->area()->selectedObject();
    if (!object) {
        print("No object is selected");
        return;
    }
    glm::vec3 position(object->position());

    stringstream ss;
    ss << setprecision(2) << fixed
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
           << "app=" << creature->appearance()
           << " "
           << "fac=" << static_cast<int>(creature->faction());
        break;
    }
    case ObjectType::Placeable: {
        auto placeable = static_pointer_cast<Placeable>(object);
        ss << " "
           << "app=" << placeable->appearance();
        break;
    }
    default:
        break;
    }

    print(ss.str());
}

void Console::cmdListGlobals(string input, vector<string> tokens) {
    auto &strings = _game.globalStrings();
    for (auto &var : strings) {
        print(var.first + " = " + var.second);
    }

    auto &booleans = _game.globalBooleans();
    for (auto &var : booleans) {
        print(var.first + " = " + (var.second ? "true" : "false"));
    }

    auto &numbers = _game.globalNumbers();
    for (auto &var : numbers) {
        print(var.first + " = " + to_string(var.second));
    }

    auto &locations = _game.globalLocations();
    for (auto &var : locations) {
        print(str(boost::format("%s = (%.04f, %.04f, %.04f, %.04f") %
                  var.first %
                  var.second->position().x %
                  var.second->position().y %
                  var.second->position().z %
                  var.second->facing()));
    }
}

void Console::cmdListLocals(string input, vector<string> tokens) {
    auto object = _game.module()->area()->selectedObject();
    if (!object) {
        print("No object is selected");
        return;
    }

    auto &booleans = object->localBooleans();
    for (auto &var : booleans) {
        print(to_string(var.first) + " -> " + (var.second ? "true" : "false"));
    }

    auto &numbers = object->localNumbers();
    for (auto &var : numbers) {
        print(to_string(var.first) + " -> " + to_string(var.second));
    }
}

void Console::cmdListAnim(string input, vector<string> tokens) {
    auto object = _game.module()->area()->selectedObject();
    if (!object) {
        object = _game.party().getLeader();
        if (!object) {
            print("No object is selected");
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

void Console::cmdPlayAnim(string input, vector<string> tokens) {
    if (tokens.size() < 2) {
        print("Usage: playanim anim_name");
        return;
    }
    auto object = _game.module()->area()->selectedObject();
    if (!object) {
        object = _game.party().getLeader();
        if (!object) {
            print("No object is selected");
            return;
        }
    }
    auto model = static_pointer_cast<ModelSceneNode>(object->sceneNode());
    model->playAnimation(tokens[1], AnimationProperties::fromFlags(AnimationFlags::loop));
}

void Console::cmdKill(string input, vector<string> tokens) {
    auto object = _game.module()->area()->selectedObject();
    if (!object) {
        print("No object is selected");
        return;
    }
    auto effect = _game.effectFactory().newDamage(100000, DamageType::Universal, nullptr);
    object->applyEffect(move(effect), DurationType::Instant);
}

void Console::cmdAddItem(string input, vector<string> tokens) {
    if (tokens.size() < 2) {
        print("Usage: additem item_tpl [size]");
        return;
    }
    auto object = _game.module()->area()->selectedObject();
    if (!object) {
        object = _game.party().getLeader();
        if (!object) {
            print("No object is selected");
            return;
        }
    }
    int stackSize = static_cast<int>(tokens.size()) > 2 ? stoi(tokens[2]) : 1;
    object->addItem(tokens[1], stackSize);
}

void Console::cmdGiveXP(string input, vector<string> tokens) {
    if (tokens.size() < 2) {
        print("Usage: givexp amount");
        return;
    }

    auto object = _game.module()->area()->selectedObject();
    if (!object) {
        object = _game.party().getLeader();
    }
    if (!object || object->type() != ObjectType::Creature) {
        print("No creature is selected");
        return;
    }

    int amount = stoi(tokens[1]);
    static_pointer_cast<Creature>(object)->giveXP(amount);
}

void Console::cmdWarp(string input, vector<string> tokens) {
    if (tokens.size() < 2) {
        print("Usage: warp module");
        return;
    }
    _game.loadModule(tokens[1]);
}

void Console::cmdListRoutine(string input, vector<string> tokens) {
    auto query = tokens.size() > 1ll ? tokens[1] : "";
    auto &routines = _game.routines();
    for (int i = 0; i < routines.getNumRoutines(); ++i) {
        auto &routine = routines.get(i);
        if (!query.empty() && !boost::contains(routine.name(), query)) {
            continue;
        }
        auto text = routine.name();
        for (int j = 0; j < routine.getArgumentCount(); ++j) {
            auto argType = routine.getArgumentType(j);
            switch (argType) {
            case VariableType::Int:
                text += " int";
                break;
            case VariableType::Float:
                text += " float";
                break;
            case VariableType::String:
                text += " string";
                break;
            case VariableType::Vector:
                text += " vector";
                break;
            case VariableType::Object:
                text += " object";
                break;
            case VariableType::Effect:
                text += " effect";
                break;
            case VariableType::Event:
                text += " event";
                break;
            case VariableType::Location:
                text += " location";
                break;
            case VariableType::Talent:
                text += " talent";
                break;
            case VariableType::Action:
                text += " action";
                break;
            default:
                text += " UNKNOWN";
                break;
            }
        }
        print(text);
    }
}

void Console::cmdExec(string input, vector<string> tokens) {
    if (tokens.size() < 3) {
        print("Usage: exec caller_id routine_name [arguments], e.g. exec 10 ActionMoveAwayFromObject 20 0 40.0");
        return;
    }
    auto callerId = stoi(tokens[1]);
    auto routineName = tokens[2];

    auto &routines = _game.routines();
    int routineIdx = routines.getIndexByName(routineName);
    if (routineIdx == -1) {
        print(str(boost::format("Routine '%s' not found") % routineName));
        return;
    }
    auto &routine = routines.get(routineIdx);

    ExecutionContext ctx;
    ctx.routines = &routines;
    ctx.callerId = callerId;

    vector<Variable> args;
    for (int i = 0; i < routine.getArgumentCount() && i < tokens.size() - 3; ++i) {
        auto argType = routine.getArgumentType(i);
        if (argType == VariableType::Int) {
            args.push_back(Variable::ofInt(stoi(tokens[i + 3])));
        } else if (argType == VariableType::Float) {
            args.push_back(Variable::ofInt(stof(tokens[i + 3])));
        } else if (argType == VariableType::String) {
            args.push_back(Variable::ofString(tokens[i + 3]));
        } else if (argType == VariableType::Object) {
            args.push_back(Variable::ofObject(stoi(tokens[i + 3])));
        } else {
            print(str(boost::format("Argument %d is of unsupported type %d") % i % static_cast<int>(argType)));
            return;
        }
    }

    auto result = routine.invoke(move(args), ctx);
    if (routine.returnType() != VariableType::Void) {
        print(routineName + " -> " + result.toString());
    }
}

void Console::cmdShowWalkmesh(string input, vector<string> tokens) {
    if (tokens.size() < 2) {
        print("Usage: showwalkmesh 1|0");
        return;
    }
    bool show = stoi(tokens[1]);
    setShowWalkmesh(show);
}

void Console::cmdShowTriggers(string input, vector<string> tokens) {
    if (tokens.size() < 2) {
        print("Usage: showtriggers 1|0");
        return;
    }
    bool show = stoi(tokens[1]);
    setShowTriggers(show);
}

void Console::cmdHelp(string input, vector<string> tokens) {
    for (auto &cmd : _commands) {
        auto text = cmd.name;
        if (!cmd.alias.empty()) {
            text += " (" + cmd.alias + ")";
        }
        text += ": " + cmd.description;
        print(text);
    }
}

void Console::print(const string &text) {
    _output.push_front(text);
    trimOutput();
    _outputOffset = 0;
}

void Console::trimOutput() {
    for (int i = static_cast<int>(_output.size()) - kMaxOutputLineCount; i > 0; --i) {
        _output.pop_back();
    }
}

} // namespace kotor

} // namespace reone
