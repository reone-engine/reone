/*
 * Copyright (c) 2020-2023 The reone project contributors
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

#include "reone/game/game.h"

#include "reone/audio/context.h"
#include "reone/audio/di/services.h"
#include "reone/audio/files.h"
#include "reone/audio/player.h"
#include "reone/graphics/context.h"
#include "reone/graphics/di/services.h"
#include "reone/graphics/format/tgawriter.h"
#include "reone/graphics/lipanimations.h"
#include "reone/graphics/meshes.h"
#include "reone/graphics/models.h"
#include "reone/graphics/pipeline.h"
#include "reone/graphics/renderbuffer.h"
#include "reone/graphics/shaders.h"
#include "reone/graphics/textures.h"
#include "reone/graphics/uniforms.h"
#include "reone/graphics/walkmeshes.h"
#include "reone/graphics/window.h"
#include "reone/gui/gui.h"
#include "reone/movie/format/bikreader.h"
#include "reone/resource/2da.h"
#include "reone/resource/2das.h"
#include "reone/resource/di/services.h"
#include "reone/resource/format/erfreader.h"
#include "reone/resource/format/erfwriter.h"
#include "reone/resource/format/gffwriter.h"
#include "reone/resource/gffs.h"
#include "reone/resource/resources.h"
#include "reone/scene/di/services.h"
#include "reone/scene/graphs.h"
#include "reone/script/di/services.h"
#include "reone/script/scripts.h"
#include "reone/system/binarywriter.h"
#include "reone/system/collectionutil.h"
#include "reone/system/di/services.h"
#include "reone/system/exception/validation.h"
#include "reone/system/logutil.h"
#include "reone/system/pathutil.h"

#include "reone/game/combat.h"
#include "reone/game/cursors.h"
#include "reone/game/debug.h"
#include "reone/game/di/services.h"
#include "reone/game/dialogs.h"
#include "reone/game/location.h"
#include "reone/game/object/factory.h"
#include "reone/game/party.h"
#include "reone/game/resourcelayout.h"
#include "reone/game/script/routines.h"
#include "reone/game/soundsets.h"
#include "reone/game/surfaces.h"

using namespace std;

using namespace reone::audio;
using namespace reone::graphics;
using namespace reone::gui;
using namespace reone::movie;
using namespace reone::resource;
using namespace reone::scene;
using namespace reone::script;

namespace reone {

namespace game {

static constexpr char kModulesDirectoryName[] = "modules";

void Game::init() {
    // Surfaces
    auto walkableSurfaces = _services.game.surfaces.getWalkableSurfaces();
    auto walkcheckSurfaces = _services.game.surfaces.getWalkcheckSurfaces();
    auto lineOfSightSurfaces = _services.game.surfaces.getLineOfSightSurfaces();
    for (auto &sceneName : _services.scene.graphs.sceneNames()) {
        auto &scene = _services.scene.graphs.get(sceneName);
        scene.setWalkableSurfaces(walkableSurfaces);
        scene.setWalkcheckSurfaces(walkcheckSurfaces);
        scene.setLineOfSightSurfaces(lineOfSightSurfaces);
    }

    _moduleNames = _services.game.resourceLayout.moduleNames();

    setCursorType(CursorType::Default);

    auto routines = make_unique<Routines>(*this, _services);
    _scriptRunner = make_unique<ScriptRunner>(*routines, _services.script.scripts);

    auto map = make_unique<Map>(*this, _services);
    auto console = make_unique<Console>(*this, _services);
    auto profileOverlay = make_unique<ProfileOverlay>(_services, _options);

    console->init();
    profileOverlay->init();

    if (isTSL()) {
        _mainMenuMusicResRef = "mus_sion";
        _charGenMusicResRef = "mus_main";
        _charGenLoadScreenResRef = "load_default";

        _guiColorBase = glm::vec3(0.192157f, 0.768627f, 0.647059f);
        _guiColorHilight = glm::vec3(0.768627f, 0.768627f, 0.686275f);
        _guiColorDisabled = glm::vec3(0.513725f, 0.513725f, 0.415686f);

        routines->initForTSL();
        map->setArrowResRef("mm_barrow_p");

    } else {
        _mainMenuMusicResRef = "mus_theme_cult";
        _charGenMusicResRef = "mus_theme_rep";
        _charGenLoadScreenResRef = "load_chargen";

        _guiColorBase = glm::vec3(0.0f, 0.639216f, 0.952941f);
        _guiColorHilight = glm::vec3(0.980392f, 1.0f, 0.0f);
        _guiColorDisabled = glm::vec3(0.0f, 0.349020f, 0.549020f);

        routines->initForKotOR();
        map->setArrowResRef("mm_barrow");
    }

    _screen = Screen::MainMenu;

    _routines = move(routines);
    _map = move(map);
    _console = move(console);
    _profileOverlay = move(profileOverlay);

    _services.graphics.window.setEventHandler(this);
}

int Game::run() {
    playVideo("legal");
    openMainMenu();

    _ticks = _services.system.clock.ticks();

    while (!_quit) {
        uint32_t ticks = _services.system.clock.ticks();
        float dt = (ticks - _ticks) / 1000.0f;
        _ticks = ticks;

        mainLoopIteration(dt * _gameSpeed);
    }

    return 0;
}

void Game::mainLoopIteration(float dt) {
    _services.graphics.window.processEvents(_quit);
    if (_quit) {
        return;
    }
    if (!_services.graphics.window.isInFocus()) {
        return;
    }
    update(dt);
    drawAll();
}

void Game::update(float dt) {
    if (_movie) {
        updateMovie(dt);
        return;
    }

    updateMusic();

    if (!_nextModule.empty()) {
        loadNextModule();
    }

    updateCamera(dt);

    bool updModule = !_movie && _module && (_screen == Screen::InGame || _screen == Screen::Conversation);
    if (updModule && !_paused) {
        _module->update(dt);
        _combat.update(dt);
    }

    GUI *gui = getScreenGUI();
    if (gui) {
        gui->update(dt);
    }
    updateSceneGraph(dt);

    _profileOverlay->update(dt);

    int x, y;
    auto state = _services.graphics.window.mouseState(&x, &y);
    auto pressed = state & SDL_BUTTON(1);
    if (_cursor) {
        _cursor->setPosition(glm::ivec2(x, y));
        _cursor->setPressed(pressed);
    }
}

void Game::drawAll() {
    _services.graphics.context.clearColorDepth();

    if (_movie) {
        _movie->render();
    } else {
        drawWorld();
        drawGUI();
        _profileOverlay->draw();
        _cursor->draw();
    }

    _services.graphics.window.swapBuffers();
}

void Game::loadModule(const string &name, string entry) {
    info("Load module '" + name + "'");

    withLoadingScreen("load_" + name, [this, &name, &entry]() {
        loadInGameMenus();

        /*
        _services.game.soundSets.invalidate();
        _services.graphics.textures.invalidate();
        _services.graphics.models.invalidate();
        _services.graphics.walkmeshes.invalidate();
        _services.graphics.lipAnimations.invalidate();
        _services.audio.files.invalidate();
        _services.script.scripts.invalidate();
        */

        try {
            if (_module) {
                _module->area()->runOnExitScript();
                _module->area()->unloadParty();
            }

            _services.game.resourceLayout.loadModuleResources(name);

            if (_loadScreen) {
                _loadScreen->setProgress(50);
            }

            drawAll();

            _services.scene.graphs.get(kSceneMain).clear();

            auto maybeModule = _loadedModules.find(name);
            if (maybeModule != _loadedModules.end()) {
                _module = maybeModule->second;
            } else {
                _module = _objectFactory.newModule();

                shared_ptr<Gff> ifo(_services.resource.gffs.get("module", ResourceType::Ifo));
                if (!ifo) {
                    throw ValidationException("Module IFO file not found");
                }

                _module->load(name, *ifo);
                _loadedModules.insert(make_pair(name, _module));
            }

            if (_party.isEmpty()) {
                loadDefaultParty();
            }

            _module->loadParty(entry);

            info("Module '" + name + "' loaded successfully");

            if (_loadScreen) {
                _loadScreen->setProgress(100);
            }
            drawAll();

            string musicName(_module->area()->music());
            playMusic(musicName);

            _ticks = _services.system.clock.ticks();
            openInGame();
        } catch (const ValidationException &e) {
            error("Failed loading module '" + name + "': " + string(e.what()));
        }
    });
}

void Game::loadDefaultParty() {
    string member1, member2, member3;
    _party.defaultMembers(member1, member2, member3);

    if (!member1.empty()) {
        shared_ptr<Creature> player(_objectFactory.newCreature());
        player->loadFromBlueprint(member1);
        player->setTag(kObjectTagPlayer);
        player->setImmortal(true);
        _party.addMember(kNpcPlayer, player);
        _party.setPlayer(player);
    }
    if (!member2.empty()) {
        shared_ptr<Creature> companion(_objectFactory.newCreature());
        companion->loadFromBlueprint(member2);
        companion->setImmortal(true);
        _party.addMember(0, companion);
    }
    if (!member3.empty()) {
        shared_ptr<Creature> companion(_objectFactory.newCreature());
        companion->loadFromBlueprint(member3);
        companion->setImmortal(true);
        _party.addMember(1, companion);
    }
}

void Game::setCursorType(CursorType type) {
    if (_cursorType == type) {
        return;
    }
    if (type == CursorType::None) {
        _cursor.reset();
        _services.graphics.window.showCursor(true);
    } else {
        _cursor = _services.game.cursors.get(type);
        _services.graphics.window.showCursor(false);
    }
    _cursorType = type;
}

void Game::playVideo(const string &name) {
    _movie = _services.movie.movies.get(name);
    if (!_movie) {
        return;
    }

    if (_music) {
        _music->stop();
        _music.reset();
    }
}

void Game::playMusic(const string &resRef) {
    if (_musicResRef == resRef) {
        return;
    }
    if (_music) {
        _music->stop();
        _music.reset();
    }
    _musicResRef = resRef;
}

void Game::drawWorld() {
    auto &scene = _services.scene.graphs.get(kSceneMain);
    auto output = _services.graphics.pipeline.draw(scene, glm::ivec2(_options.graphics.width, _options.graphics.height));
    if (!output) {
        return;
    }
    _services.graphics.uniforms.setGeneral([](auto &general) {
        general.resetGlobals();
        general.resetLocals();
    });
    _services.graphics.shaders.use(_services.graphics.shaders.simpleTexture());
    _services.graphics.textures.bind(*output);
    _services.graphics.meshes.quadNDC().draw();
}

void Game::toggleInGameCameraType() {
    switch (_cameraType) {
    case CameraType::FirstPerson:
        if (_party.getLeader()) {
            _cameraType = CameraType::ThirdPerson;
        }
        break;
    case CameraType::ThirdPerson: {
        _module->player().stopMovement();
        shared_ptr<Area> area(_module->area());
        auto &thirdPerson = area->getCamera<ThirdPersonCamera>(CameraType::ThirdPerson);
        auto &firstPerson = area->getCamera<FirstPersonCamera>(CameraType::FirstPerson);
        firstPerson.setPosition(thirdPerson.sceneNode()->getOrigin());
        firstPerson.setFacing(thirdPerson.facing());
        _cameraType = CameraType::FirstPerson;
        break;
    }
    default:
        break;
    }

    setRelativeMouseMode(_cameraType == CameraType::FirstPerson);

    _module->area()->updateRoomVisibility();
}

Camera *Game::getActiveCamera() const {
    if (!_module) {
        return nullptr;
    }
    shared_ptr<Area> area(_module->area());
    if (!area) {
        return nullptr;
    }
    return &area->getCamera(_cameraType);
}

shared_ptr<Object> Game::getObjectById(uint32_t id) const {
    switch (id) {
    case kObjectSelf:
        throw invalid_argument("id is invalid");
    case kObjectInvalid:
        return nullptr;
    default:
        return _objectFactory.getObjectById(id);
    }
}

void Game::drawGUI() {
    switch (_screen) {
    case Screen::InGame:
        if (_cameraType == CameraType::ThirdPerson) {
            drawHUD();
        }
        if (_console->isOpen()) {
            _console->draw();
        }
        break;

    default: {
        GUI *gui = getScreenGUI();
        if (gui) {
            gui->draw();
        }
        break;
    }
    }
}

void Game::updateMovie(float dt) {
    _movie->update(dt);

    if (_movie->isFinished()) {
        _movie.reset();
    }
}

void Game::updateMusic() {
    if (_musicResRef.empty()) {
        return;
    }
    if (_music && _music->isPlaying()) {
        _music->update();
    } else {
        _music = _services.audio.player.play(_musicResRef, AudioType::Music);
    }
}

void Game::loadNextModule() {
    loadModule(_nextModule, _nextEntry);

    _nextModule.clear();
    _nextEntry.clear();
}

void Game::quit() {
    _quit = true;
}

void Game::stopMovement() {
    getActiveCamera()->stopMovement();
    _module->player().stopMovement();
}

void Game::scheduleModuleTransition(const string &moduleName, const string &entry) {
    _nextModule = moduleName;
    _nextEntry = entry;
}

void Game::updateCamera(float dt) {
    switch (_screen) {
    case Screen::Conversation: {
        int cameraId;
        CameraType cameraType = getConversationCamera(cameraId);
        if (cameraType == CameraType::Static) {
            _module->area()->setStaticCamera(cameraId);
        }
        _cameraType = cameraType;
        break;
    }
    case Screen::InGame:
        if (_cameraType != CameraType::FirstPerson && _cameraType != CameraType::ThirdPerson) {
            _cameraType = CameraType::ThirdPerson;
        }
        break;
    default:
        break;
    }
    Camera *camera = getActiveCamera();
    if (camera) {
        camera->update(dt);

        glm::vec3 listenerPosition;
        if (_cameraType == CameraType::ThirdPerson) {
            shared_ptr<Creature> partyLeader(_party.getLeader());
            if (partyLeader) {
                listenerPosition = partyLeader->position() + 1.7f; // TODO: height based on appearance
            }
        } else {
            listenerPosition = camera->sceneNode()->getOrigin();
        }
        _services.audio.context.setListenerPosition(move(listenerPosition));
    }
}

void Game::updateSceneGraph(float dt) {
    const Camera *camera = getActiveCamera();
    if (!camera) {
        return;
    }
    auto &sceneGraph = _services.scene.graphs.get(kSceneMain);
    sceneGraph.setActiveCamera(camera->sceneNode().get());
    sceneGraph.setUpdateRoots(!_paused);
    sceneGraph.setDrawWalkmeshes(isShowWalkmeshEnabled());
    sceneGraph.setDrawTriggers(isShowTriggersEnabled());
    sceneGraph.update(dt);
}

bool Game::handle(const SDL_Event &event) {
    if (_profileOverlay->handle(event))
        return true;

    if (!_movie) {
        GUI *gui = getScreenGUI();
        if (gui && gui->handle(event)) {
            return true;
        }
        switch (_screen) {
        case Screen::InGame: {
            if (_console->handle(event)) {
                return true;
            }
            if (_party.handle(event)) {
                return true;
            }
            Camera *camera = getActiveCamera();
            if (camera && camera->handle(event)) {
                return true;
            }
            if (_module->handle(event)) {
                return true;
            }
            break;
        }
        default:
            break;
        }
    }
    switch (event.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (handleMouseButtonDown(event.button))
            return true;
        break;
    case SDL_KEYDOWN:
        if (handleKeyDown(event.key))
            return true;
        break;
    default:
        break;
    }

    return false;
}

bool Game::handleMouseButtonDown(const SDL_MouseButtonEvent &event) {
    if (event.button != SDL_BUTTON_LEFT)
        return false;

    if (_movie) {
        _movie->finish();
        return true;
    }

    return false;
}

bool Game::handleKeyDown(const SDL_KeyboardEvent &event) {
    if (event.repeat)
        return false;

    switch (event.keysym.sym) {
    case SDLK_MINUS:
        if (_options.game.developer && _gameSpeed > 1.0f) {
            _gameSpeed = glm::max(1.0f, _gameSpeed - 1.0f);
            return true;
        }
        break;

    case SDLK_EQUALS:
        if (_options.game.developer && _gameSpeed < 8.0f) {
            _gameSpeed = glm::min(8.0f, _gameSpeed + 1.0f);
            return true;
        }
        break;

    case SDLK_v:
        if (_options.game.developer && _screen == Screen::InGame) {
            toggleInGameCameraType();
            return true;
        }
        break;

    default:
        break;
    }

    return false;
}

bool Game::getGlobalBoolean(const string &name) const {
    return getFromLookupOrElse(_globalBooleans, name, false);
}

int Game::getGlobalNumber(const string &name) const {
    return getFromLookupOrElse(_globalNumbers, name, 0);
}

string Game::getGlobalString(const string &name) const {
    static string empty;
    return getFromLookupOrElse(_globalStrings, name, empty);
}

shared_ptr<Location> Game::getGlobalLocation(const string &name) const {
    return getFromLookupOrNull(_globalLocations, name);
}

void Game::setGlobalBoolean(const string &name, bool value) {
    _globalBooleans[name] = value;
}

void Game::setGlobalNumber(const string &name, int value) {
    _globalNumbers[name] = value;
}

void Game::setGlobalString(const string &name, const string &value) {
    _globalStrings[name] = value;
}

void Game::setGlobalLocation(const string &name, const shared_ptr<Location> &location) {
    _globalLocations[name] = location;
}

void Game::setPaused(bool paused) {
    _paused = paused;
}

void Game::setRelativeMouseMode(bool relative) {
    _services.graphics.window.setRelativeMouseMode(relative);
}

void Game::withLoadingScreen(const string &imageResRef, const function<void()> &block) {
    if (!_loadScreen) {
        _loadScreen = tryLoadGUI<LoadingScreen>();
    }
    if (_loadScreen) {
        _loadScreen->setImage(imageResRef);
        _loadScreen->setProgress(0);
    }
    changeScreen(Screen::Loading);
    drawAll();
    block();
}

void Game::openMainMenu() {
    if (!_mainMenu) {
        _mainMenu = tryLoadGUI<MainMenu>();
    }
    if (!_saveLoad) {
        _saveLoad = tryLoadGUI<SaveLoad>();
    }
    playMusic(_mainMenuMusicResRef);
    changeScreen(Screen::MainMenu);
}

void Game::openInGame() {
    changeScreen(Screen::InGame);
}

void Game::openInGameMenu(InGameMenuTab tab) {
    setCursorType(CursorType::Default);
    switch (tab) {
    case InGameMenuTab::Equipment:
        _inGame->openEquipment();
        break;
    case InGameMenuTab::Inventory:
        _inGame->openInventory();
        break;
    case InGameMenuTab::Character:
        _inGame->openCharacter();
        break;
    case InGameMenuTab::Abilities:
        _inGame->openAbilities();
        break;
    case InGameMenuTab::Messages:
        _inGame->openMessages();
        break;
    case InGameMenuTab::Journal:
        _inGame->openJournal();
        break;
    case InGameMenuTab::Map:
        _inGame->openMap();
        break;
    case InGameMenuTab::Options:
        _inGame->openOptions();
        break;
    default:
        break;
    }
    changeScreen(Screen::InGameMenu);
}

void Game::openContainer(const shared_ptr<Object> &container) {
    stopMovement();
    setRelativeMouseMode(false);
    setCursorType(CursorType::Default);
    _container->open(container);
    changeScreen(Screen::Container);
}

void Game::openPartySelection(const PartySelectionContext &ctx) {
    stopMovement();
    setRelativeMouseMode(false);
    setCursorType(CursorType::Default);
    _partySelect->prepare(ctx);
    changeScreen(Screen::PartySelection);
}

void Game::openSaveLoad(SaveLoadMode mode) {
    setRelativeMouseMode(false);
    setCursorType(CursorType::Default);
    _saveLoad->setMode(mode);
    _saveLoad->refresh();
    changeScreen(Screen::SaveLoad);
}

void Game::openLevelUp() {
    setRelativeMouseMode(false);
    setCursorType(CursorType::Default);
    _charGen->startLevelUp();
    changeScreen(Screen::CharacterGeneration);
}

void Game::startCharacterGeneration() {
    withLoadingScreen(_charGenLoadScreenResRef, [this]() {
        if (!_charGen) {
            _charGen = tryLoadGUI<CharacterGeneration>();
        }
        _loadScreen->setProgress(100);
        drawAll();
        playMusic(_charGenMusicResRef);
        changeScreen(Screen::CharacterGeneration);
    });
}

void Game::startDialog(const shared_ptr<Object> &owner, const string &resRef) {
    shared_ptr<Gff> dlg(_services.resource.gffs.get(resRef, ResourceType::Dlg));
    if (!dlg) {
        warn("Game: conversation not found: " + resRef);
        return;
    }

    stopMovement();
    setRelativeMouseMode(false);
    setCursorType(CursorType::Default);
    changeScreen(Screen::Conversation);

    auto dialog = _services.game.dialogs.get(resRef);
    bool computerConversation = dialog->conversationType == ConversationType::Computer;
    _conversation = computerConversation ? _computer.get() : static_cast<Conversation *>(_dialog.get());
    _conversation->start(dialog, owner);
}

void Game::resumeConversation() {
    _conversation->resume();
}

void Game::pauseConversation() {
    _conversation->pause();
}

void Game::loadInGameMenus() {
    if (!_hud) {
        _hud = tryLoadGUI<HUD>();
    }
    if (!_inGame) {
        _inGame = tryLoadGUI<InGameMenu>();
    }
    if (!_dialog) {
        _dialog = tryLoadGUI<DialogGUI>();
    }
    if (!_computer) {
        _computer = tryLoadGUI<ComputerGUI>();
    }
    if (!_container) {
        _container = tryLoadGUI<ContainerGUI>();
    }
    if (!_partySelect) {
        _partySelect = tryLoadGUI<PartySelection>();
    }
}

void Game::changeScreen(Screen screen) {
    GUI *gui = getScreenGUI();
    if (gui) {
        gui->resetFocus();
    }
    _screen = screen;
}

GUI *Game::getScreenGUI() const {
    switch (_screen) {
    case Screen::MainMenu:
        return _mainMenu.get();
    case Screen::Loading:
        return _loadScreen.get();
    case Screen::CharacterGeneration:
        return _charGen.get();
    case Screen::InGame:
        return _cameraType == game::CameraType::ThirdPerson ? _hud.get() : nullptr;
    case Screen::InGameMenu:
        return _inGame.get();
    case Screen::Conversation:
        return _conversation;
    case Screen::Container:
        return _container.get();
    case Screen::PartySelection:
        return _partySelect.get();
    case Screen::SaveLoad:
        return _saveLoad.get();
    default:
        return nullptr;
    }
}

void Game::setBarkBubbleText(string text, float duration) {
    _hud->barkBubble().setBarkText(text, duration);
}

void Game::onModuleSelected(const string &module) {
    _mainMenu->onModuleSelected(module);
}

void Game::drawHUD() {
    _hud->draw();
}

CameraType Game::getConversationCamera(int &cameraId) const {
    return _conversation->getCamera(cameraId);
}

} // namespace game

} // namespace reone
