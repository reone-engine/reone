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

#include "creature.h"

#include "../../audio/player.h"
#include "../../common/log.h"
#include "../../common/random.h"
#include "../../common/streamutil.h"
#include "../../common/timer.h"
#include "../../graphics/model/models.h"
#include "../../graphics/texture/textures.h"
#include "../../resource/resources.h"
#include "../../resource/strings.h"
#include "../../scene/types.h"
#include "../../script/types.h"

#include "../animationutil.h"
#include "../footstepsounds.h"
#include "../game.h"
#include "../portraits.h"
#include "../surfaces.h"

#include "objectfactory.h"

using namespace std;

using namespace reone::audio;
using namespace reone::graphics;
using namespace reone::resource;
using namespace reone::scene;
using namespace reone::script;

namespace reone {

namespace game {

static constexpr int kStrRefRemains = 38151;
static constexpr float kKeepPathDuration = 1000.0f;

static string g_talkDummyNode("talkdummy");

void Creature::Path::selectNextPoint() {
    size_t pointCount = points.size();
    if (pointIdx < pointCount) {
        pointIdx++;
    }
}

Creature::Creature(
    uint32_t id,
    Game *game,
    ObjectFactory *objectFactory,
    SceneGraph *sceneGraph
) :
    SpatialObject(id, ObjectType::Creature, game, objectFactory, sceneGraph) {
}

void Creature::loadFromGIT(const GffStruct &gffs) {
    string templateResRef(boost::to_lower_copy(gffs.getString("TemplateResRef")));
    loadFromBlueprint(templateResRef);
    loadTransformFromGIT(gffs);
}

void Creature::loadFromBlueprint(const string &resRef) {
    shared_ptr<GffStruct> utc(_game->services().resource().resources().getGFF(resRef, ResourceType::Utc));
    if (utc) {
        loadUTC(*utc);
        loadAppearance();
    }
}

void Creature::loadAppearance() {
    shared_ptr<TwoDA> appearances(_game->services().resource().resources().get2DA("appearance"));
    _modelType = parseModelType(appearances->getString(_appearance, "modeltype"));
    _walkSpeed = appearances->getFloat(_appearance, "walkdist");
    _runSpeed = appearances->getFloat(_appearance, "rundist");
    _footstepType = appearances->getInt(_appearance, "footsteptype", -1);

    if (_portraitId > 0) {
        _portrait = _game->services().portraits().getTextureByIndex(_portraitId);
    } else {
        _portrait = _game->services().portraits().getTextureByAppearance(_appearance);
    }

    updateModel();
}

Creature::ModelType Creature::parseModelType(const string &s) const {
    if (s == "S" || s == "L") {
        return ModelType::Creature;
    } else if (s == "F") {
        return ModelType::Droid;
    } else if (s == "B") {
        return ModelType::Character;
    }

    throw logic_error("Unsupported model type: " + s);
}

void Creature::updateModel() {
    if (_sceneNode) {
        _sceneGraph->removeRoot(_sceneNode);
    }
    shared_ptr<ModelSceneNode> model(buildModel());
    if (model) {
        model->setCullable(true);
        model->setDrawDistance(32.0f);
        if (!_stunt) {
            model->setLocalTransform(_transform);
        }
        _sceneGraph->addRoot(model);
        _animDirty = true;
    }
    _sceneNode = model;
}

void Creature::loadTransformFromGIT(const GffStruct &gffs) {
    _position[0] = gffs.getFloat("XPosition");
    _position[1] = gffs.getFloat("YPosition");
    _position[2] = gffs.getFloat("ZPosition");

    float cosine = gffs.getFloat("XOrientation");
    float sine = gffs.getFloat("YOrientation");
    _orientation = glm::quat(glm::vec3(0.0f, 0.0f, -glm::atan(cosine, sine)));

    updateTransform();
}

bool Creature::isSelectable() const {
    bool hasDropableItems = false;
    for (auto &item : _items) {
        if (item->isDropable()) {
            hasDropableItems = true;
            break;
        }
    }
    return !_dead || hasDropableItems;
}

void Creature::update(float dt) {
    SpatialObject::update(dt);
    updateModelAnimation();
    updateHealth();
    updateCombat(dt);
}

void Creature::updateModelAnimation() {
    auto model = static_pointer_cast<ModelSceneNode>(_sceneNode);
    if (!model) return;

    if (_animFireForget) {
        if (!model->isAnimationFinished()) return;

        _animFireForget = false;
        _animDirty = true;
    }
    if (!_animDirty) return;

    if (_animAction) {
        _animAction->complete();
        _animAction = nullptr;
    }

    shared_ptr<Animation> anim;
    shared_ptr<Animation> talkAnim;

    switch (_movementType) {
        case MovementType::Run:
            anim = model->model()->getAnimation(getRunAnimation());
            break;
        case MovementType::Walk:
            anim = model->model()->getAnimation(getWalkAnimation());
            break;
        default:
            if (_dead) {
                anim = model->model()->getAnimation(getDeadAnimation());
            } else if (_talking) {
                anim = model->model()->getAnimation(getTalkNormalAnimation());
                talkAnim = model->model()->getAnimation(getHeadTalkAnimation());
            } else {
                anim = model->model()->getAnimation(getPauseAnimation());
            }
            break;
    }

    if (talkAnim) {
        model->playAnimation(anim, nullptr, AnimationProperties::fromFlags(AnimationFlags::loopOverlay | AnimationFlags::propagate));
        model->playAnimation(talkAnim, _lipAnimation, AnimationProperties::fromFlags(AnimationFlags::loopOverlay | AnimationFlags::propagate));
    } else {
        model->playAnimation(anim, nullptr, AnimationProperties::fromFlags(AnimationFlags::loopBlend | AnimationFlags::propagate));
    }

    _animDirty = false;
}

void Creature::updateHealth() {
    if (_currentHitPoints > 0 || _immortal || _dead) return;

    die();
}

void Creature::updateCombat(float dt) {
    if (_combat.deactivationTimer.isSet() && _combat.deactivationTimer.advance(dt)) {
        _combat.active = false;
        _combat.debilitated = false;
    }
}

void Creature::clearAllActions() {
    SpatialObject::clearAllActions();
    setMovementType(MovementType::None);
}

void Creature::playAnimation(AnimationType type, AnimationProperties properties, PlayAnimationAction *actionToComplete) {
    // If animation is looping by type and action duration is -1.0, set flags accordingly
    bool looping = isAnimationLooping(type) && (!actionToComplete || actionToComplete->durationSeconds() == -1.0f);
    if (looping) {
        properties.flags |= AnimationFlags::loop;
    }

    // If animation is not supported or is looping, complete the action immediately
    string animName(getAnimationName(type));
    if (actionToComplete && (animName.empty() || looping)) {
        actionToComplete->complete();
    }
    if (animName.empty()) return;

    playAnimation(animName, move(properties), actionToComplete);
}

void Creature::playAnimation(const string &name, AnimationProperties properties, Action *actionToComplete) {
    bool fireForget = !(properties.flags & AnimationFlags::loop);

    doPlayAnimation(fireForget, [&]() {
        auto model = static_pointer_cast<ModelSceneNode>(_sceneNode);
        if (!model) return;

        _animAction = actionToComplete;

        model->playAnimation(name, properties);
    });
}

void Creature::doPlayAnimation(bool fireForget, const function<void()> &callback) {
    if (!_sceneNode || _movementType != MovementType::None) return;

    callback();

    if (fireForget) {
        _animFireForget = true;
    }
}

void Creature::playAnimation(const shared_ptr<Animation> &anim, AnimationProperties properties) {
    bool fireForget = !(properties.flags & AnimationFlags::loop);

    doPlayAnimation(fireForget, [&]() {
        auto model = static_pointer_cast<ModelSceneNode>(_sceneNode);
        if (!model) return;

        model->playAnimation(anim, nullptr, properties);
    });
}

void Creature::playAnimation(CombatAnimation anim, CreatureWieldType wield, int variant) {
    string animName(getAnimationName(anim, wield, variant));
    if (!animName.empty()) {
        playAnimation(animName, AnimationProperties::fromFlags(AnimationFlags::blend));
    }
}

bool Creature::equip(const string &resRef) {
    shared_ptr<Item> item(_objectFactory->newItem());
    item->loadFromBlueprint(resRef);

    bool equipped = false;

    if (item->isEquippable(InventorySlot::body)) {
        equipped = equip(InventorySlot::body, item);
    } else if (item->isEquippable(InventorySlot::rightWeapon)) {
        equipped = equip(InventorySlot::rightWeapon, item);
    }

    return equipped;
}

bool Creature::equip(int slot, const shared_ptr<Item> &item) {
    if (!item->isEquippable(slot)) return false;

    _equipment[slot] = item;
    item->setEquipped(true);

    if (_sceneNode) {
        updateModel();

        auto model = static_pointer_cast<ModelSceneNode>(_sceneNode);
        if (model) {
            if (slot == InventorySlot::rightWeapon) {
                auto weapon = static_pointer_cast<ModelSceneNode>(model->getAttachment("rhand"));
                if (weapon && weapon->model()->classification() == Model::Classification::Lightsaber) {
                    weapon->playAnimation("powerup");
                }
            }
        }
    }

    return true;
}

void Creature::unequip(const shared_ptr<Item> &item) {
    for (auto &equipped : _equipment) {
        if (equipped.second == item) {
            item->setEquipped(false);
            _equipment.erase(equipped.first);

            if (_sceneNode) {
                updateModel();
            }
            break;
        }
    }
}

shared_ptr<Item> Creature::getEquippedItem(int slot) const {
    auto equipped = _equipment.find(slot);
    return equipped != _equipment.end() ? equipped->second : nullptr;
}

bool Creature::isSlotEquipped(int slot) const {
    return _equipment.find(slot) != _equipment.end();
}

void Creature::setMovementType(MovementType type) {
    if (_movementType == type) return;

    _movementType = type;
    _animDirty = true;
    _animFireForget = false;
}

void Creature::setPath(const glm::vec3 &dest, vector<glm::vec3> &&points, uint32_t timeFound) {
    int pointIdx = 0;
    if (_path) {
        bool lastPointReached = _path->pointIdx == _path->points.size();
        if (lastPointReached) {
            float nearestDist = INFINITY;
            for (int i = 0; i < points.size(); ++i) {
                float dist = glm::distance2(_path->destination, points[i]);
                if (dist < nearestDist) {
                    nearestDist = dist;
                    pointIdx = i;
                }
            }
        } else {
            const glm::vec3 &nextPoint = _path->points[_path->pointIdx];
            for (int i = 0; i < points.size(); ++i) {
                if (points[i] == nextPoint) {
                    pointIdx = i;
                    break;
                }
            }
        }
    }
    auto path = make_unique<Path>();
    path->destination = dest;
    path->points = points;
    path->timeFound = timeFound;
    path->pointIdx = pointIdx;

    _path = move(path);
}

void Creature::clearPath() {
    _path.reset();
}

glm::vec3 Creature::getSelectablePosition() const {
    auto model = static_pointer_cast<ModelSceneNode>(_sceneNode);
    if (!model) return _position;

    shared_ptr<ModelNode> talkDummy(model->model()->getNodeByNameRecursive(g_talkDummyNode));
    if (!talkDummy || _dead) return model->getWorldCenterOfAABB();

    return (model->absoluteTransform() * talkDummy->absoluteTransform())[3];
}

float Creature::getAttackRange() const {
    float result = kDefaultAttackRange;

    shared_ptr<Item> item(getEquippedItem(InventorySlot::rightWeapon));
    if (item && item->attackRange() > kDefaultAttackRange) {
        result = item->attackRange();
    }

    return result;
}

bool Creature::isLevelUpPending() const {
    return _xp >= getNeededXP();
}

int Creature::getNeededXP() const {
    int level = _attributes.getAggregateLevel();
    return level * (level + 1) * 500;
}

void Creature::runSpawnScript() {
    if (!_onSpawn.empty()) {
        _game->services().scriptRunner().run(_onSpawn, _id, kObjectInvalid);
    }
}

void Creature::runEndRoundScript() {
    if (!_onEndRound.empty()) {
        _game->services().scriptRunner().run(_onEndRound, _id, kObjectInvalid);
    }
}

void Creature::giveXP(int amount) {
    _xp += amount;
}

void Creature::playSound(SoundSetEntry entry, bool positional) {
    if (!_soundSet) return;

    auto maybeSound = _soundSet->find(entry);
    if (maybeSound != _soundSet->end()) {
        glm::vec3 position(_position + 1.7f);
        _game->services().audio().player().play(maybeSound->second, AudioType::Sound, false, 1.0f, positional, position);
    }
}

void Creature::die() {
    _currentHitPoints = 0;
    _dead = true;
    _name = _game->services().resource().strings().get(kStrRefRemains);

    debug(boost::format("Creature %s is dead") % _tag, 2);

    playSound(SoundSetEntry::Dead);
    playAnimation(getDieAnimation());
    runDeathScript();
}

void Creature::runDeathScript() {
    if (!_onDeath.empty()) {
        _game->services().scriptRunner().run(_onDeath, _id, kObjectInvalid);
    }
}

CreatureWieldType Creature::getWieldType() const {
    auto rightWeapon = getEquippedItem(InventorySlot::rightWeapon);
    auto leftWeapon = getEquippedItem(InventorySlot::leftWeapon);

    if (rightWeapon && leftWeapon) {
        return (rightWeapon->weaponWield() == WeaponWield::BlasterPistol) ? CreatureWieldType::DualPistols : CreatureWieldType::DualSwords;
    } else if (rightWeapon) {
        switch (rightWeapon->weaponWield()) {
            case WeaponWield::SingleSword:
                return CreatureWieldType::SingleSword;
            case WeaponWield::DoubleBladedSword:
                return CreatureWieldType::DoubleBladedSword;
            case WeaponWield::BlasterPistol:
                return CreatureWieldType::BlasterPistol;
            case WeaponWield::BlasterRifle:
                return CreatureWieldType::BlasterRifle;
            case WeaponWield::HeavyWeapon:
                return CreatureWieldType::HeavyWeapon;
            case WeaponWield::StunBaton:
            default:
                return CreatureWieldType::StunBaton;
        }
    }

    return CreatureWieldType::HandToHand;
}

void Creature::startTalking(const shared_ptr<LipAnimation> &animation) {
    if (!_talking || _lipAnimation != animation) {
        _lipAnimation = animation;
        _talking = true;
        _animDirty = true;
    }
}

void Creature::stopTalking() {
    if (_talking || _lipAnimation) {
        _lipAnimation.reset();
        _talking = false;
        _animDirty = true;
    }
}

void Creature::onObjectSeen(const shared_ptr<SpatialObject> &object) {
    _perception.seen.insert(object);
    _perception.lastPerception = PerceptionType::Seen;
    _perception.lastPerceived = object;
    runOnNoticeScript();
}

void Creature::runOnNoticeScript() {
    if (!_onNotice.empty()) {
        _game->services().scriptRunner().run(_onNotice, _id, _perception.lastPerceived->id());
    }
}

void Creature::onObjectVanished(const shared_ptr<SpatialObject> &object) {
    _perception.seen.erase(object);
    _perception.lastPerception = PerceptionType::NotSeen;
    _perception.lastPerceived = object;
    runOnNoticeScript();
}

void Creature::onObjectHeard(const shared_ptr<SpatialObject> &object) {
    _perception.heard.insert(object);
    _perception.lastPerception = PerceptionType::Heard;
    _perception.lastPerceived = object;
    runOnNoticeScript();
}

void Creature::onObjectInaudible(const shared_ptr<SpatialObject> &object) {
    _perception.heard.erase(object);
    _perception.lastPerception = PerceptionType::NotHeard;
    _perception.lastPerceived = object;
    runOnNoticeScript();
}

void Creature::activateCombat() {
    _combat.active = true;
    if (_combat.deactivationTimer.isSet()) {
        _combat.deactivationTimer.cancel();
    }
}

void Creature::deactivateCombat(float delay) {
    if (_combat.active && !_combat.deactivationTimer.isSet()) {
        _combat.deactivationTimer.setTimeout(delay);
    }
}

bool Creature::isTwoWeaponFighting() const {
    return static_cast<bool>(getEquippedItem(InventorySlot::leftWeapon));
}

shared_ptr<SpatialObject> Creature::getAttemptedAttackTarget() const {
    shared_ptr<SpatialObject> result;

    auto attackAction = dynamic_pointer_cast<ObjectAction>(getCurrentAction());
    if (attackAction) {
        result = static_pointer_cast<SpatialObject>(attackAction->object());
    }

    return move(result);
}

int Creature::getAttackBonus(bool offHand) const {
    auto rightWeapon(getEquippedItem(InventorySlot::rightWeapon));
    auto leftWeapon(getEquippedItem(InventorySlot::leftWeapon));
    auto weapon = offHand ? leftWeapon : rightWeapon;

    int modifier;
    if (weapon && weapon->isRanged()) {
        modifier = _attributes.getAbilityModifier(Ability::Dexterity);
    } else {
        modifier = _attributes.getAbilityModifier(Ability::Strength);
    }

    int penalty;
    if (rightWeapon && leftWeapon) {
        // TODO: support Dueling and Two-Weapon Fighting feats
        penalty = offHand ? 10 : 6;
    } else {
        penalty = 0;
    }

    return _attributes.getAggregateAttackBonus() + modifier - penalty;
}

int Creature::getDefense() const {
    return _attributes.getDefense();
}

void Creature::getMainHandDamage(int &min, int &max) const {
    getWeaponDamage(InventorySlot::rightWeapon, min, max);
}

void Creature::getWeaponDamage(int slot, int &min, int &max) const {
    auto weapon = getEquippedItem(slot);

    if (!weapon) {
        min = 1;
        max = 1;
    } else {
        min = weapon->numDice();
        max = weapon->numDice() * weapon->dieToRoll();
    }

    int modifier;
    if (weapon && weapon->isRanged()) {
        modifier = _attributes.getAbilityModifier(Ability::Dexterity);
    } else {
        modifier = _attributes.getAbilityModifier(Ability::Strength);
    }
    min += modifier;
    max += modifier;
}

void Creature::getOffhandDamage(int &min, int &max) const {
    getWeaponDamage(InventorySlot::leftWeapon, min, max);
}

void Creature::setAppliedForce(glm::vec3 force) {
    if (_sceneNode && _sceneNode->type() == SceneNodeType::Model) {
        static_pointer_cast<ModelSceneNode>(_sceneNode)->setAppliedForce(force);
    }
}

void Creature::onEventSignalled(const string &name) {
    if (name == "snd_footstep" && _footstepType != -1 && _walkmeshMaterial != -1) {
        shared_ptr<FootstepTypeSounds> sounds(_game->services().footstepSounds().get(_footstepType));
        if (sounds) {
            const Surface &surface = _game->services().surfaces().getSurface(_walkmeshMaterial);
            vector<shared_ptr<AudioStream>> materialSounds;
            if (surface.sound == "DT") {
                materialSounds = sounds->dirt;
            } else if (surface.sound == "GR") {
                materialSounds = sounds->grass;
            } else if (surface.sound == "ST") {
                materialSounds = sounds->stone;
            } else if (surface.sound == "WD") {
                materialSounds = sounds->wood;
            } else if (surface.sound == "WT") {
                materialSounds = sounds->water;
            } else if (surface.sound == "CP") {
                materialSounds = sounds->carpet;
            } else if (surface.sound == "MT") {
                materialSounds = sounds->metal;
            } else if (surface.sound == "LV") {
                materialSounds = sounds->leaves;
            }
            int index = random(0, 3);
            if (index < static_cast<int>(materialSounds.size())) {
                shared_ptr<AudioStream> sound(materialSounds[index]);
                if (sound) {
                    _game->services().audio().player().play(sound, AudioType::Sound, false, 1.0f, true, _position);
                }
            }
        }
    }
}

void Creature::giveGold(int amount) {
    _gold += amount;
}

void Creature::takeGold(int amount) {
    _gold -= amount;
}

bool Creature::navigateTo(const glm::vec3 &dest, bool run, float distance, float dt) {
    if (_movementRestricted) return false;

    float distToDest2 = getDistanceTo2(glm::vec2(dest));
    if (distToDest2 <= distance * distance) {
        setMovementType(Creature::MovementType::None);
        clearPath();
        return true;
    }

    bool updPath = true;
    if (_path) {
        uint32_t now = SDL_GetTicks();
        if (_path->destination == dest || now - _path->timeFound <= kKeepPathDuration) {
            advanceOnPath(run, dt);
            updPath = false;
        }
    }
    if (updPath) {
        updatePath(dest);
    }

    return false;
}

void Creature::advanceOnPath(bool run, float dt) {
    const glm::vec3 &origin = _position;
    size_t pointCount = _path->points.size();
    glm::vec3 dest;
    float distToDest;

    if (_path->pointIdx == pointCount) {
        dest = _path->destination;
        distToDest = glm::distance2(origin, dest);

    } else {
        const glm::vec3 &nextPoint = _path->points[_path->pointIdx];
        float distToNextPoint = glm::distance2(origin, nextPoint);
        float distToPathDest = glm::distance2(origin, _path->destination);

        if (distToPathDest < distToNextPoint) {
            dest = _path->destination;
            distToDest = distToPathDest;
            _path->pointIdx = static_cast<int>(pointCount);

        } else {
            dest = nextPoint;
            distToDest = distToNextPoint;
        }
    }

    if (distToDest <= 1.0f) {
        _path->selectNextPoint();
    } else {
        shared_ptr<Creature> creature(_game->services().objectFactory().getObjectById<Creature>(_id));
        if (_game->module()->area()->moveCreatureTowards(creature, dest, run, dt)) {
            setMovementType(run ? Creature::MovementType::Run : Creature::MovementType::Walk);
            setAppliedForce(glm::vec3(glm::normalize(glm::vec2(dest - origin)), 0.0f));
        } else {
            setMovementType(Creature::MovementType::None);
            setAppliedForce(glm::vec3(0.0f));
        }
    }
}

void Creature::updatePath(const glm::vec3 &dest) {
    vector<glm::vec3> points(_game->module()->area()->pathfinder().findPath(_position, dest));
    uint32_t now = SDL_GetTicks();
    setPath(dest, move(points), now);
}

} // namespace game

} // namespace reone
