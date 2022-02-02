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

#include "selectoverlay.h"

#include "../../game/action/attack.h"
#include "../../game/action/factory.h"
#include "../../game/action/usefeat.h"
#include "../../game/action/useskill.h"
#include "../../game/d20/feat.h"
#include "../../game/d20/feats.h"
#include "../../game/d20/skill.h"
#include "../../game/d20/skills.h"
#include "../../game/party.h"
#include "../../game/reputes.h"
#include "../../game/services.h"
#include "../../graphics/context.h"
#include "../../graphics/font.h"
#include "../../graphics/fonts.h"
#include "../../graphics/mesh.h"
#include "../../graphics/meshes.h"
#include "../../graphics/shaders.h"
#include "../../graphics/texture.h"
#include "../../graphics/textures.h"
#include "../../graphics/uniforms.h"
#include "../../graphics/window.h"
#include "../../resource/resources.h"

#include "../kotor.h"

using namespace std;

using namespace reone::game;
using namespace reone::graphics;
using namespace reone::resource;

namespace reone {

namespace kotor {

static constexpr int kOffsetToReticle = 8;
static constexpr int kTitleBarWidth = 250;
static constexpr int kTitleBarPadding = 6;
static constexpr int kHealthBarHeight = 6;
static constexpr int kNumActionSlots = 3;
static constexpr int kActionBarMargin = 3;
static constexpr int kActionBarPadding = 3;
static constexpr int kActionWidth = 35;
static constexpr int kActionHeight = 59;

static string g_attackIcon("i_attack");

SelectionOverlay::SelectionOverlay(
    Game &game,
    Services &services) :
    _game(game),
    _services(services) {
    _actionSlots.resize(kNumActionSlots);
}

void SelectionOverlay::load() {
    _font = _services.fonts.get("dialogfont16x16");
    _friendlyReticle = _services.textures.get("friendlyreticle", TextureUsage::GUI);
    _friendlyReticle2 = _services.textures.get("friendlyreticle2", TextureUsage::GUI);
    _hostileReticle = _services.textures.get("hostilereticle", TextureUsage::GUI);
    _hostileReticle2 = _services.textures.get("hostilereticle2", TextureUsage::GUI);
    _friendlyScroll = _services.textures.get("lbl_miscroll_f", TextureUsage::GUI);
    _hostileScroll = _services.textures.get("lbl_miscroll_h", TextureUsage::GUI);
    _hilightedScroll = _services.textures.get("lbl_miscroll_hi", TextureUsage::GUI);
    _reticleHeight = _friendlyReticle2->height();
}

bool SelectionOverlay::handle(const SDL_Event &event) {
    switch (event.type) {
    case SDL_MOUSEMOTION:
        return handleMouseMotion(event.motion);
    case SDL_MOUSEBUTTONDOWN:
        return handleMouseButtonDown(event.button);
    case SDL_MOUSEWHEEL:
        return handleMouseWheel(event.wheel);
    default:
        return false;
    }
}

bool SelectionOverlay::handleMouseMotion(const SDL_MouseMotionEvent &event) {
    _selectedActionSlot = -1;

    if (!_selectedObject)
        return false;

    for (int i = 0; i < kNumActionSlots; ++i) {
        float x, y;
        getActionScreenCoords(i, x, y);
        if (event.x >= x && event.y >= y && event.x < x + kActionWidth && event.y < y + kActionHeight) {
            _selectedActionSlot = i;
            return true;
        }
    }

    return false;
}

bool SelectionOverlay::handleMouseButtonDown(const SDL_MouseButtonEvent &event) {
    if (event.button != SDL_BUTTON_LEFT)
        return false;
    if (_selectedActionSlot == -1 || _selectedActionSlot >= _actionSlots.size())
        return false;

    shared_ptr<Creature> leader(_game.party().getLeader());
    if (!leader)
        return false;

    shared_ptr<Area> area(_game.module()->area());
    auto selectedObject = area->selectedObject();
    if (!selectedObject)
        return false;

    const ActionSlot &slot = _actionSlots[_selectedActionSlot];
    if (slot.indexSelected >= slot.actions.size())
        return false;

    const ContextAction &action = slot.actions[slot.indexSelected];
    switch (action.type) {
    case ActionType::AttackObject:
        leader->addAction(_game.actionFactory().newAttack(selectedObject, leader->getAttackRange(), true));
        break;
    case ActionType::UseFeat:
        leader->addAction(_game.actionFactory().newUseFeat(selectedObject, action.feat));
        break;
    case ActionType::UseSkill:
        leader->addAction(_game.actionFactory().newUseSkill(selectedObject, action.skill));
        break;
    default:
        break;
    }

    return true;
}

bool SelectionOverlay::handleMouseWheel(const SDL_MouseWheelEvent &event) {
    if (_selectedActionSlot == -1 || _selectedActionSlot >= _actionSlots.size())
        return false;

    ActionSlot &slot = _actionSlots[_selectedActionSlot];
    size_t numSlotActions = slot.actions.size();

    if (event.y > 0) {
        if (slot.indexSelected-- == 0) {
            slot.indexSelected = static_cast<uint32_t>(numSlotActions - 1);
        }
    } else {
        if (++slot.indexSelected == numSlotActions) {
            slot.indexSelected = 0;
        }
    }

    return true;
}

void SelectionOverlay::update() {
    // TODO: update on selection change only

    _hilightedObject.reset();
    _hilightedHostile = false;

    _selectedObject.reset();
    _selectedHostile = false;

    shared_ptr<Module> module(_game.module());
    shared_ptr<Area> area(module->area());

    game::Camera *camera = _game.getActiveCamera();
    glm::mat4 projection(camera->sceneNode()->camera()->projection());
    glm::mat4 view(camera->sceneNode()->camera()->view());

    auto hilightedObject = area->hilightedObject();
    if (hilightedObject) {
        _hilightedScreenCoords = area->getSelectableScreenCoords(hilightedObject, projection, view);

        if (_hilightedScreenCoords.z < 1.0f) {
            _hilightedObject = hilightedObject;

            auto hilightedCreature = dynamic_pointer_cast<Creature>(hilightedObject);
            if (hilightedCreature) {
                _hilightedHostile = !hilightedCreature->isDead() && _services.reputes.getIsEnemy(*(_game.party().getLeader()), *hilightedCreature);
            }
        }
    }

    auto selectedObject = area->selectedObject();
    if (selectedObject) {
        _selectedScreenCoords = area->getSelectableScreenCoords(selectedObject, projection, view);

        if (_selectedScreenCoords.z < 1.0f) {
            _selectedObject = selectedObject;

            for (int i = 0; i < kNumActionSlots; ++i) {
                _actionSlots[i].actions.clear();
            }
            vector<ContextAction> actions(module->getContextActions(selectedObject));
            _hasActions = !actions.empty();
            if (_hasActions) {
                for (auto &action : actions) {
                    switch (action.type) {
                    case ActionType::AttackObject:
                    case ActionType::UseFeat:
                        _actionSlots[0].actions.push_back(action);
                        break;
                    case ActionType::UseSkill:
                        _actionSlots[1].actions.push_back(action);
                        break;
                    default:
                        break;
                    }
                }
            }
            for (int i = 0; i < kNumActionSlots; ++i) {
                if (_actionSlots[i].indexSelected >= _actionSlots[i].actions.size()) {
                    _actionSlots[i].indexSelected = 0;
                }
            }

            auto selectedCreature = dynamic_pointer_cast<Creature>(selectedObject);
            if (selectedCreature) {
                _selectedHostile = !selectedCreature->isDead() && _services.reputes.getIsEnemy(*_game.party().getLeader(), *selectedCreature);
            }
        }
    }
}

void SelectionOverlay::draw() {
    _services.graphicsContext.withBlending(BlendMode::Normal, [this]() {
        if (_hilightedObject) {
            drawReticle(_hilightedHostile ? _hostileReticle : _friendlyReticle, _hilightedScreenCoords);
        }
        if (_selectedObject) {
            drawReticle(_selectedHostile ? _hostileReticle2 : _friendlyReticle2, _selectedScreenCoords);
            drawActionBar();
            drawTitleBar();
            drawHealthBar();
        }
    });
}

void SelectionOverlay::drawReticle(shared_ptr<Texture> texture, const glm::vec3 &screenCoords) {
    _services.textures.bind(*texture);

    const GraphicsOptions &opts = _game.options().graphics;
    int width = texture->width();
    int height = texture->height();

    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, glm::vec3((opts.width * screenCoords.x) - width / 2, (opts.height * (1.0f - screenCoords.y)) - height / 2, 0.0f));
    transform = glm::scale(transform, glm::vec3(width, height, 1.0f));

    _services.uniforms.setGeneral([this, transform](auto &general) {
        general.resetLocals();
        general.projection = _services.window.getOrthoProjection();
        general.model = move(transform);
    });
    _services.shaders.use(_services.shaders.gui());
    _services.meshes.quad().draw();
}

void SelectionOverlay::drawTitleBar() {
    if (_selectedObject->name().empty())
        return;

    const GraphicsOptions &opts = _game.options().graphics;
    float barHeight = _font->height() + kTitleBarPadding;
    {
        float x = opts.width * _selectedScreenCoords.x - kTitleBarWidth / 2;
        float y = opts.height * (1.0f - _selectedScreenCoords.y) - _reticleHeight / 2.0f - barHeight - kOffsetToReticle - kHealthBarHeight - 1.0f;

        if (_hasActions) {
            y -= kActionHeight + 2 * kActionBarMargin;
        }
        glm::mat4 transform(1.0f);
        transform = glm::translate(transform, glm::vec3(x, y, 0.0f));
        transform = glm::scale(transform, glm::vec3(kTitleBarWidth, barHeight, 1.0f));

        _services.uniforms.setGeneral([this, transform](auto &general) {
            general.resetLocals();
            general.projection = _services.window.getOrthoProjection();
            general.model = move(transform);
            general.color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            general.alpha = 0.5f;
        });
        _services.shaders.use(_services.shaders.simpleColor());
        _services.meshes.quad().draw();
    }
    {
        float x = opts.width * _selectedScreenCoords.x;
        float y = opts.height * (1.0f - _selectedScreenCoords.y) - (_reticleHeight + barHeight) / 2 - kOffsetToReticle - kHealthBarHeight - 1.0f;
        if (_hasActions) {
            y -= kActionHeight + 2 * kActionBarMargin;
        }
        glm::vec3 position(x, y, 0.0f);
        _font->draw(_selectedObject->name(), position, getColorFromSelectedObject());
    }
}

void SelectionOverlay::drawHealthBar() {
    const GraphicsOptions &opts = _game.options().graphics;
    float x = opts.width * _selectedScreenCoords.x - kTitleBarWidth / 2;
    float y = opts.height * (1.0f - _selectedScreenCoords.y) - _reticleHeight / 2.0f - kHealthBarHeight - kOffsetToReticle;
    float w = glm::clamp(_selectedObject->currentHitPoints() / static_cast<float>(_selectedObject->hitPoints()), 0.0f, 1.0f) * kTitleBarWidth;

    if (_hasActions) {
        y -= kActionHeight + 2 * kActionBarMargin;
    }
    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, glm::vec3(x, y, 0.0f));
    transform = glm::scale(transform, glm::vec3(w, kHealthBarHeight, 1.0f));

    _services.uniforms.setGeneral([this, transform](auto &general) {
        general.resetLocals();
        general.projection = _services.window.getOrthoProjection();
        general.model = move(transform);
        general.color = glm::vec4(getColorFromSelectedObject(), 1.0f);
    });
    _services.shaders.use(_services.shaders.simpleColor());
    _services.meshes.quad().draw();
}

void SelectionOverlay::drawActionBar() {
    if (!_hasActions)
        return;

    for (int i = 0; i < kNumActionSlots; ++i) {
        drawActionFrame(i);
        drawActionIcon(i);
    }
}

void SelectionOverlay::drawActionFrame(int index) {
    shared_ptr<Texture> frameTexture;
    if (index == _selectedActionSlot) {
        frameTexture = _hilightedScroll;
    } else if (_selectedHostile) {
        frameTexture = _hostileScroll;
    } else {
        frameTexture = _friendlyScroll;
    }
    _services.textures.bind(*frameTexture);

    float frameX, frameY;
    getActionScreenCoords(index, frameX, frameY);

    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, glm::vec3(frameX, frameY, 0.0f));
    transform = glm::scale(transform, glm::vec3(kActionWidth, kActionHeight, 1.0f));

    _services.uniforms.setGeneral([this, transform](auto &general) {
        general.resetLocals();
        general.projection = _services.window.getOrthoProjection();
        general.model = move(transform);
    });
    _services.shaders.use(_services.shaders.gui());
    _services.meshes.quad().draw();
}

bool SelectionOverlay::getActionScreenCoords(int index, float &x, float &y) const {
    if (!_selectedObject)
        return false;

    const GraphicsOptions &opts = _game.options().graphics;
    x = opts.width * _selectedScreenCoords.x + (static_cast<float>(index - 1) - 0.5f) * kActionWidth + (index - 1) * kActionBarMargin;
    y = opts.height * (1.0f - _selectedScreenCoords.y) - _reticleHeight / 2.0f - kActionHeight - kOffsetToReticle - kActionBarMargin;

    return true;
}

void SelectionOverlay::drawActionIcon(int index) {
    const ActionSlot &slot = _actionSlots[index];
    if (slot.indexSelected >= slot.actions.size())
        return;

    shared_ptr<Texture> texture;
    const ContextAction &action = slot.actions[slot.indexSelected];
    switch (action.type) {
    case ActionType::AttackObject:
        texture = _services.textures.get(g_attackIcon, TextureUsage::GUI);
        break;
    case ActionType::UseFeat: {
        shared_ptr<Feat> feat(_services.feats.get(action.feat));
        if (feat) {
            texture = feat->icon;
        }
        break;
    }
    case ActionType::UseSkill: {
        shared_ptr<Skill> skill(_services.skills.get(action.skill));
        if (skill) {
            texture = skill->icon;
        }
        break;
    }
    default:
        break;
    }
    if (!texture)
        return;

    _services.textures.bind(*texture);

    float frameX, frameY;
    getActionScreenCoords(index, frameX, frameY);

    const GraphicsOptions &opts = _game.options().graphics;
    float y = opts.height * (1.0f - _selectedScreenCoords.y) - (_reticleHeight + kActionHeight + kActionWidth) / 2.0f - kOffsetToReticle - kActionBarMargin;

    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, glm::vec3(frameX, y, 0.0f));
    transform = glm::scale(transform, glm::vec3(kActionWidth, kActionWidth, 1.0f));

    _services.uniforms.setGeneral([this, transform](auto &general) {
        general.resetLocals();
        general.projection = _services.window.getOrthoProjection();
        general.model = move(transform);
    });
    _services.shaders.use(_services.shaders.gui());
    _services.meshes.quad().draw();
}

glm::vec3 SelectionOverlay::getColorFromSelectedObject() const {
    static glm::vec3 red(1.0f, 0.0f, 0.0f);

    return (_selectedObject && _selectedHostile) ? red : _game.getGUIColorBase();
}

} // namespace kotor

} // namespace reone
