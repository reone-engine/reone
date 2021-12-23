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
#include "../../common/exception/validation.h"
#include "../../common/logutil.h"
#include "../../common/randomutil.h"
#include "../../common/streamutil.h"
#include "../../common/timer.h"
#include "../../graphics/models.h"
#include "../../graphics/textures.h"
#include "../../resource/2da.h"
#include "../../resource/2das.h"
#include "../../resource/gffs.h"
#include "../../resource/resources.h"
#include "../../resource/strings.h"
#include "../../scene/graphs.h"
#include "../../scene/types.h"
#include "../../script/types.h"

#include "../action/objectaction.h"
#include "../animationutil.h"
#include "../d20/classes.h"
#include "../footstepsounds.h"
#include "../game.h"
#include "../portraits.h"
#include "../script/runner.h"
#include "../services.h"
#include "../soundsets.h"
#include "../surfaces.h"

#include "factory.h"

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

static const string g_headHookNode("headhook");
static const string g_maskHookNode("gogglehook");
static const string g_rightHandNode("rhand");
static const string g_leftHandNode("lhand");

void Creature::Path::selectNextPoint() {
    size_t pointCount = points.size();
    if (pointIdx < pointCount) {
        pointIdx++;
    }
}

void Creature::loadFromGIT(const GffStruct &gffs) {
    string templateResRef(boost::to_lower_copy(gffs.getString("TemplateResRef")));
    loadFromBlueprint(templateResRef);
    loadTransformFromGIT(gffs);
}

void Creature::loadFromBlueprint(const string &resRef) {
    shared_ptr<GffStruct> utc(_services.gffs.get(resRef, ResourceType::Utc));
    if (utc) {
        loadUTC(*utc);
        loadAppearance();
    }
}

void Creature::loadAppearance() {
    shared_ptr<TwoDA> appearances(_services.twoDas.get("appearance"));
    if (!appearances) {
        throw ValidationException("appearance 2DA is not found");
    }

    _modelType = parseModelType(appearances->getString(_appearance, "modeltype"));
    _walkSpeed = appearances->getFloat(_appearance, "walkdist", 1.0f);
    _runSpeed = appearances->getFloat(_appearance, "rundist", 1.0f);
    _footstepType = appearances->getInt(_appearance, "footsteptype", -1);

    if (_portraitId > 0) {
        _portrait = _services.portraits.getTextureByIndex(_portraitId);
    } else {
        _portrait = _services.portraits.getTextureByAppearance(_appearance);
    }

    auto modelSceneNode = buildModel();
    if (modelSceneNode) {
        finalizeModel(*modelSceneNode);
        _sceneNode = move(modelSceneNode);
        _sceneNode->setUser(*this);
        _sceneNode->setLocalTransform(_transform);
    }

    _animDirty = true;
}

Creature::ModelType Creature::parseModelType(const string &s) const {
    if (s == "S" || s == "L") {
        return ModelType::Creature;
    } else if (s == "F") {
        return ModelType::Droid;
    } else if (s == "B") {
        return ModelType::Character;
    }

    throw logic_error("Model type '" + s + "' is not supported");
}

void Creature::updateModel() {
    if (!_sceneNode) {
        return;
    }
    auto bodyModelName = getBodyModelName();
    if (bodyModelName.empty()) {
        return;
    }
    auto replacement = _services.models.get(bodyModelName);
    if (!replacement) {
        return;
    }
    auto model = static_pointer_cast<ModelSceneNode>(_sceneNode);
    model->setModel(move(replacement));
    finalizeModel(*model);
    if (!_stunt) {
        model->setLocalTransform(_transform);
    }
    _animDirty = true;
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

    if (_audioSourceVoice) {
        _audioSourceVoice->update();
    }
    if (_audioSourceFootstep) {
        _audioSourceFootstep->update();
    }
}

void Creature::updateModelAnimation() {
    auto model = static_pointer_cast<ModelSceneNode>(_sceneNode);
    if (!model)
        return;

    if (_animFireForget) {
        if (!model->isAnimationFinished())
            return;

        _animFireForget = false;
        _animDirty = true;
    }
    if (!_animDirty)
        return;

    shared_ptr<Animation> anim;
    shared_ptr<Animation> talkAnim;

    switch (_movementType) {
    case MovementType::Run:
        anim = model->model().getAnimation(getRunAnimation());
        break;
    case MovementType::Walk:
        anim = model->model().getAnimation(getWalkAnimation());
        break;
    default:
        if (_dead) {
            anim = model->model().getAnimation(getDeadAnimation());
        } else if (_talking) {
            anim = model->model().getAnimation(getTalkNormalAnimation());
            talkAnim = model->model().getAnimation(getHeadTalkAnimation());
        } else {
            anim = model->model().getAnimation(getPauseAnimation());
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
    if (_currentHitPoints > 0 || _immortal || _dead)
        return;

    die();
}

void Creature::updateCombat(float dt) {
    if (_combatState.deactivationTimer.isSet() && _combatState.deactivationTimer.advance(dt)) {
        _combatState.active = false;
        _combatState.debilitated = false;
    }
}

void Creature::clearAllActions() {
    SpatialObject::clearAllActions();
    setMovementType(MovementType::None);
}

void Creature::playAnimation(AnimationType type, AnimationProperties properties) {
    // If animation is looping by type and duration is -1.0, set flags accordingly
    bool looping = isAnimationLooping(type) && properties.duration == -1.0f;
    if (looping) {
        properties.flags |= AnimationFlags::loop;
    }

    string animName(getAnimationName(type));
    if (animName.empty())
        return;

    playAnimation(animName, move(properties));
}

void Creature::playAnimation(const string &name, AnimationProperties properties) {
    bool fireForget = !(properties.flags & AnimationFlags::loop);

    doPlayAnimation(fireForget, [&]() {
        auto model = static_pointer_cast<ModelSceneNode>(_sceneNode);
        if (model) {
            model->playAnimation(name, properties);
        }
    });
}

void Creature::doPlayAnimation(bool fireForget, const function<void()> &callback) {
    if (!_sceneNode || _movementType != MovementType::None)
        return;

    callback();

    if (fireForget) {
        _animFireForget = true;
    }
}

void Creature::playAnimation(const shared_ptr<Animation> &anim, AnimationProperties properties) {
    bool fireForget = !(properties.flags & AnimationFlags::loop);

    doPlayAnimation(fireForget, [&]() {
        auto model = static_pointer_cast<ModelSceneNode>(_sceneNode);
        if (model) {
            model->playAnimation(anim, nullptr, properties);
        }
    });
}

void Creature::playAnimation(CombatAnimation anim, CreatureWieldType wield, int variant) {
    string animName(getAnimationName(anim, wield, variant));
    if (!animName.empty()) {
        playAnimation(animName, AnimationProperties::fromFlags(AnimationFlags::blend));
    }
}

bool Creature::equip(const string &resRef) {
    shared_ptr<Item> item(_game.objectFactory().newItem());
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
    if (!item->isEquippable(slot)) {
        return false;
    }

    _equipment[slot] = item;
    item->setEquipped(true);

    if (_sceneNode) {
        updateModel();

        if (slot == InventorySlot::rightWeapon) {
            auto model = static_pointer_cast<ModelSceneNode>(_sceneNode);
            auto weapon = static_pointer_cast<ModelSceneNode>(model->getAttachment("rhand"));
            if (weapon && weapon->model().classification() == MdlClassification::lightsaber) {
                weapon->playAnimation("powerup");
            }
        }
    }

    return true;
}

void Creature::unequip(const shared_ptr<Item> &item) {
    for (auto &equipped : _equipment) {
        if (equipped.second != item) {
            continue;
        }
        item->setEquipped(false);
        _equipment.erase(equipped.first);
        if (_sceneNode) {
            updateModel();
        }
        break;
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
    if (_movementType == type)
        return;

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
    if (!model)
        return _position;

    shared_ptr<ModelNode> talkDummy(model->model().getNodeByNameRecursive(g_talkDummyNode));
    if (!talkDummy || _dead)
        return model->getWorldCenterOfAABB();

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
        _game.scriptRunner().run(_onSpawn, _id, kObjectInvalid);
    }
}

void Creature::runEndRoundScript() {
    if (!_onEndRound.empty()) {
        _game.scriptRunner().run(_onEndRound, _id, kObjectInvalid);
    }
}

void Creature::giveXP(int amount) {
    _xp += amount;
}

void Creature::playSound(SoundSetEntry entry, bool positional) {
    if (!_soundSet) {
        return;
    }
    auto maybeSound = _soundSet->find(entry);
    if (maybeSound == _soundSet->end()) {
        return;
    }
    glm::vec3 position(_position + 1.7f);
    _audioSourceVoice = _services.audioPlayer.play(maybeSound->second, AudioType::Sound, false, 1.0f, positional, position);
}

void Creature::die() {
    _currentHitPoints = 0;
    _dead = true;
    _name = _services.strings.get(kStrRefRemains);

    debug(boost::format("Creature %s is dead") % _tag);

    playSound(SoundSetEntry::Dead);
    playAnimation(getDieAnimation());
    runDeathScript();
}

void Creature::runDeathScript() {
    if (!_onDeath.empty()) {
        _game.scriptRunner().run(_onDeath, _id, kObjectInvalid);
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
        _game.scriptRunner().run(_onNotice, _id, _perception.lastPerceived->id());
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
    _combatState.active = true;
    if (_combatState.deactivationTimer.isSet()) {
        _combatState.deactivationTimer.cancel();
    }
}

void Creature::deactivateCombat(float delay) {
    if (_combatState.active && !_combatState.deactivationTimer.isSet()) {
        _combatState.deactivationTimer.setTimeout(delay);
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
    if (_footstepType == -1 || _walkmeshMaterial == -1 || name != "snd_footstep") {
        return;
    }
    shared_ptr<FootstepTypeSounds> sounds(_services.footstepSounds.get(_footstepType));
    if (!sounds) {
        return;
    }
    const Surface &surface = _services.surfaces.getSurface(_walkmeshMaterial);
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
    if (index >= static_cast<int>(materialSounds.size())) {
        return;
    }
    shared_ptr<AudioStream> sound(materialSounds[index]);
    if (sound) {
        _audioSourceFootstep = _services.audioPlayer.play(sound, AudioType::Sound, false, 1.0f, true, _position);
    }
}

void Creature::giveGold(int amount) {
    _gold += amount;
}

void Creature::takeGold(int amount) {
    _gold -= amount;
}

bool Creature::navigateTo(const glm::vec3 &dest, bool run, float distance, float dt) {
    if (_movementRestricted)
        return false;

    float distToDest2 = getSquareDistanceTo(glm::vec2(dest));
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
        shared_ptr<Creature> creature(_game.objectFactory().getObjectById<Creature>(_id));
        if (_game.module()->area()->moveCreatureTowards(creature, dest, run, dt)) {
            setMovementType(run ? Creature::MovementType::Run : Creature::MovementType::Walk);
            setAppliedForce(glm::vec3(glm::normalize(glm::vec2(dest - origin)), 0.0f));
        } else {
            setMovementType(Creature::MovementType::None);
            setAppliedForce(glm::vec3(0.0f));
        }
    }
}

void Creature::updatePath(const glm::vec3 &dest) {
    vector<glm::vec3> points(_game.module()->area()->pathfinder().findPath(_position, dest));
    uint32_t now = SDL_GetTicks();
    setPath(dest, move(points), now);
}

string Creature::getAnimationName(AnimationType anim) const {
    string result;
    switch (anim) {
    case AnimationType::LoopingPause:
        return getPauseAnimation();
    case AnimationType::LoopingPause2:
        return getFirstIfCreatureModel("cpause2", "pause2");
    case AnimationType::LoopingListen:
        return "listen";
    case AnimationType::LoopingMeditate:
        return "meditate";
    case AnimationType::LoopingTalkNormal:
        return "tlknorm";
    case AnimationType::LoopingTalkPleading:
        return "tlkplead";
    case AnimationType::LoopingTalkForceful:
        return "tlkforce";
    case AnimationType::LoopingTalkLaughing:
        return getFirstIfCreatureModel("", "tlklaugh");
    case AnimationType::LoopingTalkSad:
        return "tlksad";
    case AnimationType::LoopingPauseTired:
        return "pausetrd";
    case AnimationType::LoopingFlirt:
        return "flirt";
    case AnimationType::LoopingUseComputer:
        return "usecomplp";
    case AnimationType::LoopingDance:
        return "dance";
    case AnimationType::LoopingDance1:
        return "dance1";
    case AnimationType::LoopingHorror:
        return "horror";
    case AnimationType::LoopingDeactivate:
        return getFirstIfCreatureModel("", "deactivate");
    case AnimationType::LoopingSpasm:
        return getFirstIfCreatureModel("cspasm", "spasm");
    case AnimationType::LoopingSleep:
        return "sleep";
    case AnimationType::LoopingProne:
        return "prone";
    case AnimationType::LoopingPause3:
        return getFirstIfCreatureModel("", "pause3");
    case AnimationType::LoopingWeld:
        return "weld";
    case AnimationType::LoopingDead:
        return getDeadAnimation();
    case AnimationType::LoopingTalkInjured:
        return "talkinj";
    case AnimationType::LoopingListenInjured:
        return "listeninj";
    case AnimationType::LoopingTreatInjured:
        return "treatinjlp";
    case AnimationType::LoopingUnlockDoor:
        return "unlockdr";
    case AnimationType::LoopingClosed:
        return "closed";
    case AnimationType::LoopingStealth:
        return "stealth";
    case AnimationType::FireForgetHeadTurnLeft:
        return getFirstIfCreatureModel("chturnl", "hturnl");
    case AnimationType::FireForgetHeadTurnRight:
        return getFirstIfCreatureModel("chturnr", "hturnr");
    case AnimationType::FireForgetSalute:
        return "salute";
    case AnimationType::FireForgetBow:
        return "bow";
    case AnimationType::FireForgetGreeting:
        return "greeting";
    case AnimationType::FireForgetTaunt:
        return getFirstIfCreatureModel("ctaunt", "taunt");
    case AnimationType::FireForgetVictory1:
        return getFirstIfCreatureModel("cvictory", "victory");
    case AnimationType::FireForgetInject:
        return "inject";
    case AnimationType::FireForgetUseComputer:
        return "usecomp";
    case AnimationType::FireForgetPersuade:
        return "persuade";
    case AnimationType::FireForgetActivate:
        return "activate";
    case AnimationType::LoopingChoke:
        return "choke";
    case AnimationType::FireForgetTreatInjured:
        return "treatinj";
    case AnimationType::FireForgetOpen:
        return "open";
    case AnimationType::LoopingReady:
        return getAnimationName(CombatAnimation::Ready, getWieldType(), 0);

    case AnimationType::LoopingWorship:
    case AnimationType::LoopingGetLow:
    case AnimationType::LoopingGetMid:
    case AnimationType::LoopingPauseDrunk:
    case AnimationType::LoopingDeadProne:
    case AnimationType::LoopingKneelTalkAngry:
    case AnimationType::LoopingKneelTalkSad:
    case AnimationType::LoopingCheckBody:
    case AnimationType::LoopingSitAndMeditate:
    case AnimationType::LoopingSitChair:
    case AnimationType::LoopingSitChairDrink:
    case AnimationType::LoopingSitChairPazak:
    case AnimationType::LoopingSitChairComp1:
    case AnimationType::LoopingSitChairComp2:
    case AnimationType::LoopingRage:
    case AnimationType::LoopingChokeWorking:
    case AnimationType::LoopingMeditateStand:
    case AnimationType::FireForgetPauseScratchHead:
    case AnimationType::FireForgetPauseBored:
    case AnimationType::FireForgetVictory2:
    case AnimationType::FireForgetVictory3:
    case AnimationType::FireForgetThrowHigh:
    case AnimationType::FireForgetThrowLow:
    case AnimationType::FireForgetCustom01:
    case AnimationType::FireForgetForceCast:
    case AnimationType::FireForgetDiveRoll:
    case AnimationType::FireForgetScream:
    default:
        debug("CreatureAnimationResolver: unsupported animation type: " + to_string(static_cast<int>(anim)));
        return "";
    }
}

string Creature::getDieAnimation() const {
    return getFirstIfCreatureModel("cdie", "die");
}

string Creature::getFirstIfCreatureModel(string creatureAnim, string elseAnim) const {
    return _modelType == Creature::ModelType::Creature ? move(creatureAnim) : move(elseAnim);
}

string Creature::getDeadAnimation() const {
    return getFirstIfCreatureModel("cdead", "dead");
}

string Creature::getPauseAnimation() const {
    if (_modelType == Creature::ModelType::Creature)
        return "cpause1";

    // TODO: if (_lowHP) return "pauseinj"

    if (_combatState.active) {
        WeaponType type = WeaponType::None;
        WeaponWield wield = WeaponWield::None;
        getWeaponInfo(type, wield);

        int wieldNumber = getWeaponWieldNumber(wield);
        return str(boost::format("g%dr1") % wieldNumber);
    }

    return "pause1";
}

bool Creature::getWeaponInfo(WeaponType &type, WeaponWield &wield) const {
    shared_ptr<Item> item(getEquippedItem(InventorySlot::rightWeapon));
    if (item) {
        type = item->weaponType();
        wield = item->weaponWield();
        return true;
    }

    return false;
}

int Creature::getWeaponWieldNumber(WeaponWield wield) const {
    switch (wield) {
    case WeaponWield::StunBaton:
        return 1;
    case WeaponWield::SingleSword:
        return isSlotEquipped(InventorySlot::leftWeapon) ? 4 : 2;
    case WeaponWield::DoubleBladedSword:
        return 3;
    case WeaponWield::BlasterPistol:
        return isSlotEquipped(InventorySlot::leftWeapon) ? 6 : 5;
    case WeaponWield::BlasterRifle:
        return 7;
    case WeaponWield::HeavyWeapon:
        return 9;
    default:
        return 8;
    }
}

string Creature::getWalkAnimation() const {
    return getFirstIfCreatureModel("cwalk", "walk");
}

string Creature::getRunAnimation() const {
    if (_modelType == Creature::ModelType::Creature)
        return "crun";

    // TODO: if (_lowHP) return "runinj"

    if (_combatState.active) {
        WeaponType type = WeaponType::None;
        WeaponWield wield = WeaponWield::None;
        getWeaponInfo(type, wield);

        switch (wield) {
        case WeaponWield::SingleSword:
            return isSlotEquipped(InventorySlot::leftWeapon) ? "runds" : "runss";
        case WeaponWield::DoubleBladedSword:
            return "runst";
        case WeaponWield::BlasterRifle:
        case WeaponWield::HeavyWeapon:
            return "runrf";
        default:
            break;
        }
    }

    return "run";
}

string Creature::getTalkNormalAnimation() const {
    return "tlknorm";
}

string Creature::getHeadTalkAnimation() const {
    return "talk";
}

static string formatCombatAnimation(const string &format, CreatureWieldType wield, int variant) {
    return str(boost::format(format) % static_cast<int>(wield) % variant);
}

string Creature::getAnimationName(CombatAnimation anim, CreatureWieldType wield, int variant) const {
    switch (anim) {
    case CombatAnimation::Draw:
        return getFirstIfCreatureModel("", formatCombatAnimation("g%dw%d", wield, 1));
    case CombatAnimation::Ready:
        return getFirstIfCreatureModel("creadyr", formatCombatAnimation("g%dr%d", wield, 1));
    case CombatAnimation::Attack:
        return getFirstIfCreatureModel("g0a1", formatCombatAnimation("g%da%d", wield, variant));
    case CombatAnimation::Damage:
        return getFirstIfCreatureModel("cdamages", formatCombatAnimation("g%dd%d", wield, variant));
    case CombatAnimation::Dodge:
        return getFirstIfCreatureModel("cdodgeg", formatCombatAnimation("g%dg%d", wield, variant));
    case CombatAnimation::MeleeAttack:
        return getFirstIfCreatureModel("m0a1", formatCombatAnimation("m%da%d", wield, variant));
    case CombatAnimation::MeleeDamage:
        return getFirstIfCreatureModel("cdamages", formatCombatAnimation("m%dd%d", wield, variant));
    case CombatAnimation::MeleeDodge:
        return getFirstIfCreatureModel("cdodgeg", formatCombatAnimation("m%dg%d", wield, variant));
    case CombatAnimation::CinematicMeleeAttack:
        return formatCombatAnimation("c%da%d", wield, variant);
    case CombatAnimation::CinematicMeleeDamage:
        return formatCombatAnimation("c%dd%d", wield, variant);
    case CombatAnimation::CinematicMeleeParry:
        return formatCombatAnimation("c%dp%d", wield, variant);
    case CombatAnimation::BlasterAttack:
        return getFirstIfCreatureModel("b0a1", formatCombatAnimation("b%da%d", wield, variant));
    default:
        return "";
    }
}

string Creature::getActiveAnimationName() const {
    auto model = dynamic_pointer_cast<ModelSceneNode>(_sceneNode);
    if (!model)
        return "";

    return model->getActiveAnimationName();
}

shared_ptr<ModelSceneNode> Creature::buildModel() {
    string modelName(getBodyModelName());
    if (modelName.empty()) {
        return nullptr;
    }
    shared_ptr<Model> model(_services.models.get(modelName));
    if (!model) {
        return nullptr;
    }
    auto &sceneGraph = _services.sceneGraphs.get(_sceneName);
    auto sceneNode = sceneGraph.newModel(model, ModelUsage::Creature, this);
    sceneNode->setCullable(true);
    sceneNode->setDrawDistance(_game.options().graphics.drawDistance);

    return move(sceneNode);
}

void Creature::finalizeModel(ModelSceneNode &body) {
    auto &sceneGraph = _services.sceneGraphs.get(_sceneName);

    // Body texture

    string bodyTextureName(getBodyTextureName());
    if (!bodyTextureName.empty()) {
        shared_ptr<Texture> texture(_services.textures.get(bodyTextureName, TextureUsage::Diffuse));
        if (texture) {
            body.setDiffuseTexture(texture);
        }
    }

    // Mask

    shared_ptr<Model> maskModel;
    string maskModelName(getMaskModelName());
    if (!maskModelName.empty()) {
        maskModel = _services.models.get(maskModelName);
    }

    // Head

    string headModelName(getHeadModelName());
    if (!headModelName.empty()) {
        shared_ptr<Model> headModel(_services.models.get(headModelName));
        if (headModel) {
            shared_ptr<ModelSceneNode> headSceneNode(sceneGraph.newModel(headModel, ModelUsage::Creature, this));
            body.attach(g_headHookNode, headSceneNode);
            if (maskModel) {
                auto maskSceneNode = sceneGraph.newModel(maskModel, ModelUsage::Equipment, this);
                headSceneNode->attach(g_maskHookNode, move(maskSceneNode));
            }
        }
    }

    // Right weapon

    string rightWeaponModelName(getWeaponModelName(InventorySlot::rightWeapon));
    if (!rightWeaponModelName.empty()) {
        shared_ptr<Model> weaponModel(_services.models.get(rightWeaponModelName));
        if (weaponModel) {
            shared_ptr<ModelSceneNode> weaponSceneNode(sceneGraph.newModel(weaponModel, ModelUsage::Equipment, this));
            body.attach(g_rightHandNode, move(weaponSceneNode));
        }
    }

    // Left weapon

    string leftWeaponModelName(getWeaponModelName(InventorySlot::leftWeapon));
    if (!leftWeaponModelName.empty()) {
        shared_ptr<Model> weaponModel(_services.models.get(leftWeaponModelName));
        if (weaponModel) {
            shared_ptr<ModelSceneNode> weaponSceneNode(sceneGraph.newModel(weaponModel, ModelUsage::Equipment, this));
            body.attach(g_leftHandNode, move(weaponSceneNode));
        }
    }
}

string Creature::getBodyModelName() const {
    string column;

    if (_modelType == Creature::ModelType::Character) {
        column = "model";

        shared_ptr<Item> bodyItem(getEquippedItem(InventorySlot::body));
        if (bodyItem) {
            string baseBodyVar(bodyItem->baseBodyVariation());
            column += baseBodyVar;
        } else {
            column += "a";
        }

    } else {
        column = "race";
    }

    shared_ptr<TwoDA> appearance(_services.twoDas.get("appearance"));
    if (!appearance) {
        throw ValidationException("appearance 2DA is not found");
    }

    string modelName(appearance->getString(_appearance, column));
    boost::to_lower(modelName);

    return move(modelName);
}

string Creature::getBodyTextureName() const {
    string column;
    shared_ptr<Item> bodyItem(getEquippedItem(InventorySlot::body));

    if (_modelType == Creature::ModelType::Character) {
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

    shared_ptr<TwoDA> appearance(_services.twoDas.get("appearance"));
    if (!appearance) {
        throw ValidationException("appearance 2DA is not found");
    }

    string texName(boost::to_lower_copy(appearance->getString(_appearance, column)));
    if (texName.empty())
        return "";

    if (_modelType == Creature::ModelType::Character) {
        bool texFound = false;
        if (bodyItem) {
            string tmp(str(boost::format("%s%02d") % texName % bodyItem->textureVariation()));
            shared_ptr<Texture> texture(_services.textures.get(tmp, TextureUsage::Diffuse));
            if (texture) {
                texName = move(tmp);
                texFound = true;
            }
        }
        if (!texFound) {
            texName += "01";
        }
    }

    return move(texName);
}

string Creature::getHeadModelName() const {
    if (_modelType != Creature::ModelType::Character) {
        return "";
    }
    shared_ptr<TwoDA> appearance(_services.twoDas.get("appearance"));
    if (!appearance) {
        throw ValidationException("appearance 2DA is not found");
    }
    int headIdx = appearance->getInt(_appearance, "normalhead", -1);
    if (headIdx == -1) {
        return "";
    }
    shared_ptr<TwoDA> heads(_services.twoDas.get("heads"));
    if (!heads) {
        throw ValidationException("heads 2DA is not found");
    }

    string modelName(heads->getString(headIdx, "head"));
    boost::to_lower(modelName);

    return move(modelName);
}

string Creature::getMaskModelName() const {
    shared_ptr<Item> headItem(getEquippedItem(InventorySlot::head));
    if (!headItem)
        return "";

    string modelName(boost::to_lower_copy(headItem->itemClass()));
    modelName += str(boost::format("_%03d") % headItem->modelVariation());

    return move(modelName);
}

string Creature::getWeaponModelName(int slot) const {
    shared_ptr<Item> bodyItem(getEquippedItem(slot));
    if (!bodyItem)
        return "";

    string modelName(bodyItem->itemClass());
    boost::to_lower(modelName);

    modelName += str(boost::format("_%03d") % bodyItem->modelVariation());

    return move(modelName);
}

void Creature::loadUTC(const GffStruct &utc) {
    _blueprintResRef = boost::to_lower_copy(utc.getString("TemplateResRef"));
    _race = utc.getEnum("Race", RacialType::Invalid);      // index into racialtypes.2da
    _subrace = utc.getEnum("SubraceIndex", Subrace::None); // index into subrace.2da
    _appearance = utc.getInt("Appearance_Type");           // index into appearance.2da
    _gender = utc.getEnum("Gender", Gender::None);         // index into gender.2da
    _portraitId = utc.getInt("PortraitId");                // index into portrait.2da
    _tag = boost::to_lower_copy(utc.getString("Tag"));
    _conversation = boost::to_lower_copy(utc.getString("Conversation"));
    _isPC = utc.getBool("IsPC");                           // always 0
    _faction = utc.getEnum("FactionID", Faction::Invalid); // index into repute.2da
    _disarmable = utc.getBool("Disarmable");
    _plot = utc.getBool("Plot");
    _interruptable = utc.getBool("Interruptable");
    _noPermDeath = utc.getBool("NoPermDeath");
    _notReorienting = utc.getBool("NotReorienting");
    _bodyVariation = utc.getInt("BodyVariation");
    _textureVar = utc.getInt("TextureVar");
    _minOneHP = utc.getBool("Min1HP");
    _partyInteract = utc.getBool("PartyInteract");
    _walkRate = utc.getInt("WalkRate"); // index into creaturespeed.2da
    _naturalAC = utc.getInt("NaturalAC");
    _hitPoints = utc.getInt("HitPoints");
    _currentHitPoints = utc.getInt("CurrentHitPoints");
    _maxHitPoints = utc.getInt("MaxHitPoints");
    _forcePoints = utc.getInt("ForcePoints");
    _currentForce = utc.getInt("CurrentForce");
    _refBonus = utc.getInt("refbonus");
    _willBonus = utc.getInt("willbonus");
    _fortBonus = utc.getInt("fortbonus");
    _goodEvil = utc.getInt("GoodEvil");
    _challengeRating = utc.getInt("ChallengeRating");

    _onHeartbeat = boost::to_lower_copy(utc.getString("ScriptHeartbeat"));
    _onNotice = boost::to_lower_copy(utc.getString("ScriptOnNotice"));
    _onSpellAt = boost::to_lower_copy(utc.getString("ScriptSpellAt"));
    _onAttacked = boost::to_lower_copy(utc.getString("ScriptAttacked"));
    _onDamaged = boost::to_lower_copy(utc.getString("ScriptDamaged"));
    _onDisturbed = boost::to_lower_copy(utc.getString("ScriptDisturbed"));
    _onEndRound = boost::to_lower_copy(utc.getString("ScriptEndRound"));
    _onEndDialogue = boost::to_lower_copy(utc.getString("ScriptEndDialogu"));
    _onDialogue = boost::to_lower_copy(utc.getString("ScriptDialogue"));
    _onSpawn = boost::to_lower_copy(utc.getString("ScriptSpawn"));
    _onDeath = boost::to_lower_copy(utc.getString("ScriptDeath"));
    _onUserDefined = boost::to_lower_copy(utc.getString("ScriptUserDefine"));
    _onBlocked = boost::to_lower_copy(utc.getString("ScriptOnBlocked"));

    loadNameFromUTC(utc);
    loadSoundSetFromUTC(utc);
    loadBodyBagFromUTC(utc);
    loadAttributesFromUTC(utc);
    loadPerceptionRangeFromUTC(utc);

    for (auto &item : utc.getList("Equip_ItemList")) {
        equip(boost::to_lower_copy(item->getString("EquippedRes")));
    }
    for (auto &itemGffs : utc.getList("ItemList")) {
        string resRef(boost::to_lower_copy(itemGffs->getString("InventoryRes")));
        bool dropable = itemGffs->getBool("Dropable");
        addItem(resRef, 1, dropable);
    }

    // Unused fields:
    //
    // - Phenotype (not applicable, always 0)
    // - Description (not applicable)
    // - Subrace (unknown, we already use SubraceIndex)
    // - Deity (not applicable, always empty)
    // - LawfulChaotic (not applicable)
    // - ScriptRested (not applicable, mostly empty)
    // - PaletteID (toolset only)
    // - Comment (toolset only)
}

void Creature::loadNameFromUTC(const GffStruct &utc) {
    string firstName(_services.strings.get(utc.getInt("FirstName")));
    string lastName(_services.strings.get(utc.getInt("LastName")));
    if (!firstName.empty() && !lastName.empty()) {
        _name = firstName + " " + lastName;
    } else if (!firstName.empty()) {
        _name = firstName;
    }
}

void Creature::loadSoundSetFromUTC(const GffStruct &utc) {
    uint32_t soundSetIdx = utc.getUint("SoundSetFile", 0xffff);
    if (soundSetIdx == 0xffff) {
        return;
    }
    shared_ptr<TwoDA> soundSetTable(_services.twoDas.get("soundset"));
    if (!soundSetTable) {
        return;
    }
    string soundSetResRef(soundSetTable->getString(soundSetIdx, "resref"));
    if (!soundSetResRef.empty()) {
        _soundSet = _services.soundSets.get(soundSetResRef);
    }
}

void Creature::loadBodyBagFromUTC(const GffStruct &utc) {
    shared_ptr<TwoDA> bodyBags(_services.twoDas.get("bodybag"));
    if (!bodyBags) {
        return;
    }
    int bodyBag = utc.getInt("BodyBag");
    _bodyBag.name = _services.strings.get(bodyBags->getInt(bodyBag, "name"));
    _bodyBag.appearance = bodyBags->getInt(bodyBag, "appearance");
    _bodyBag.corpse = bodyBags->getBool(bodyBag, "corpse");
}

void Creature::loadAttributesFromUTC(const GffStruct &utc) {
    CreatureAttributes &attributes = _attributes;
    attributes.setAbilityScore(Ability::Strength, utc.getInt("Str"));
    attributes.setAbilityScore(Ability::Dexterity, utc.getInt("Dex"));
    attributes.setAbilityScore(Ability::Constitution, utc.getInt("Con"));
    attributes.setAbilityScore(Ability::Intelligence, utc.getInt("Int"));
    attributes.setAbilityScore(Ability::Wisdom, utc.getInt("Wis"));
    attributes.setAbilityScore(Ability::Charisma, utc.getInt("Cha"));

    for (auto &classGffs : utc.getList("ClassList")) {
        int clazz = classGffs->getInt("Class");
        int level = classGffs->getInt("ClassLevel");
        attributes.addClassLevels(_services.classes.get(static_cast<ClassType>(clazz)).get(), level);
        for (auto &spellGffs : classGffs->getList("KnownList0")) {
            auto spell = static_cast<ForcePower>(spellGffs->getUint("Spell"));
            attributes.addSpell(spell);
        }
    }

    vector<shared_ptr<GffStruct>> skillsUtc(utc.getList("SkillList"));
    for (int i = 0; i < static_cast<int>(skillsUtc.size()); ++i) {
        SkillType skill = static_cast<SkillType>(i);
        attributes.setSkillRank(skill, skillsUtc[i]->getInt("Rank"));
    }

    for (auto &featGffs : utc.getList("FeatList")) {
        auto feat = static_cast<FeatType>(featGffs->getUint("Feat"));
        _attributes.addFeat(feat);
    }
}

void Creature::loadPerceptionRangeFromUTC(const GffStruct &utc) {
    shared_ptr<TwoDA> ranges(_services.twoDas.get("ranges"));
    if (!ranges) {
        return;
    }
    int rangeIdx = utc.getInt("PerceptionRange");
    _perception.sightRange = ranges->getFloat(rangeIdx, "primaryrange");
    _perception.hearingRange = ranges->getFloat(rangeIdx, "secondaryrange");
}

} // namespace game

} // namespace reone
