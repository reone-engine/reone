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

#include "conversation.h"

#include "../../audio/files.h"
#include "../../audio/player.h"
#include "../../common/logutil.h"
#include "../../gui/control/listbox.h"
#include "../../graphics/lip/lips.h"
#include "../../graphics/model/animation.h"
#include "../../graphics/model/models.h"
#include "../../resource/resources.h"

#include "../game.h"
#include "../script/runner.h"

using namespace std;

using namespace reone::audio;
using namespace reone::gui;
using namespace reone::graphics;
using namespace reone::resource;

namespace reone {

namespace game {

static constexpr float kDefaultEntryDuration = 10.0f;

static bool g_allEntriesSkippable = false;

Conversation::Conversation(Game *game) : GameGUI(game) {
}

void Conversation::start(const shared_ptr<Dialog> &dialog, const shared_ptr<SpatialObject> &owner) {
    debug("Start " + dialog->resRef(), LogChannels::conversation);

    _dialog = dialog;
    _owner = owner;

    loadConversationBackground();
    loadCameraModel();
    onStart();
    loadStartEntry();
}

static BackgroundType getBackgroundType(ComputerType compType) {
    switch (compType) {
        case ComputerType::Rakatan:
            return BackgroundType::Computer1;
        default:
            return BackgroundType::Computer0;
    }
}

void Conversation::loadConversationBackground() {
    if (_dialog->conversationType() == ConversationType::Computer) {
        loadBackground(getBackgroundType(_dialog->computerType()));
    } else {
        loadBackground(BackgroundType::None);
    }
}

void Conversation::loadCameraModel() {
    string modelResRef(_dialog->cameraModel());
    _cameraModel = modelResRef.empty() ? nullptr : _game->models().get(modelResRef);
}

void Conversation::onStart() {
}

void Conversation::loadStartEntry() {
    int entryIdx = indexOfFirstActive(_dialog->startEntries());
    if (entryIdx == -1) {
        debug("Finish (no active start entry)", LogChannels::conversation);
        finish();
        return;
    }
    loadEntry(entryIdx, true);
}

int Conversation::indexOfFirstActive(const vector<Dialog::EntryReplyLink> &links) {
    for (auto &link : links) {
        if (link.active.empty() || evaluateCondition(link.active)) {
            return link.index;
        }
    }
    return -1;
}

bool Conversation::evaluateCondition(const string &scriptResRef) {
    return _game->scriptRunner().run(scriptResRef, _owner->id()) != 0;
}

void Conversation::finish() {
    onFinish();

    _game->openInGame();

    // Run EndConversation script
    if (!_dialog->endScript().empty()) {
        _game->scriptRunner().run(_dialog->endScript(), _owner->id());
    }
}

void Conversation::onFinish() {
}

void Conversation::loadEntry(int index, bool start) {
    debug("Load entry " + to_string(index), LogChannels::conversation);
    _currentEntry = &_dialog->getEntry(index);

    setMessage(_currentEntry->text);
    loadReplies();
    loadVoiceOver();
    scheduleEndOfEntry();
    onLoadEntry();

    // Conversation is a one-liner if there is exactly one empty reply that has no entries
    bool oneLiner = false;
    if (start && _replies.size() == 1ll) {
        const Dialog::EntryReply &reply = *_replies[0];
        oneLiner = reply.text.empty() && reply.entries.empty();
    }
    if (oneLiner) {
        _game->hud().barkBubble().setBarkText(_currentEntry->text, _entryDuration);
        debug("Dialog: finish (one-liner)");
        finish();
        return;
    }

    // Run entry script
    if (!_currentEntry->script.empty()) {
        _game->scriptRunner().run(_currentEntry->script, _owner->id());
    }
}

void Conversation::onLoadEntry() {
}

void Conversation::loadVoiceOver() {
    // Stop previous voice, if any
    if (_currentVoice) {
        _currentVoice->stop();
        _currentVoice.reset();
        _lipAnimation.reset();
    }

    // Play current voice over either from Sound or from VO_ResRef
    string voiceResRef;
    if (!_currentEntry->sound.empty()) {
        voiceResRef = _currentEntry->sound;
    }
    if (voiceResRef.empty() && !_currentEntry->voResRef.empty()) {
        voiceResRef = _currentEntry->voResRef;
    }
    if (!voiceResRef.empty()) {
        _currentVoice = _game->audioPlayer().play(voiceResRef, AudioType::Voice);
        _lipAnimation = _game->lips().get(voiceResRef);
    }
}

static string getCameraAnimationName(int ordinal) {
    return str(boost::format("cut%03dw") % (ordinal - 1200 + 1));
}

void Conversation::scheduleEndOfEntry() {
    float duration = kDefaultEntryDuration;

    if (_cameraModel && (_currentEntry->waitFlags & DialogWaitFlags::waitAnimFinish)) {
        string animName(getCameraAnimationName(_currentEntry->cameraAnimation));
        shared_ptr<Animation> animation(_cameraModel->getAnimation(animName));
        if (animation) {
            duration = animation->length();
        }
    } else if (_currentEntry->delay != -1) {
        duration = static_cast<float>(_currentEntry->delay);
    } else if (_currentVoice) {
        duration = _currentVoice->duration();
    }

    _entryEnded = false;
    _entryDuration = duration;
    _endEntryTimer.setTimeout(duration);
}

void Conversation::loadReplies() {
    _replies.clear();
    for (auto &link : _currentEntry->replies) {
        if (link.active.empty() || evaluateCondition(link.active)) {
            _replies.push_back(&_dialog->getReply(link.index));
        }
    }

    // If there is only one empty reply, pick it automatically when the current entry ends
    _autoPickFirstReply = _replies.size() == 1ll && _replies.front()->text.empty();

    refreshReplies();
}

static string getReplyText(const Dialog::EntryReply &reply, int index) {
    return str(boost::format("%d. %s") % (index + 1) % (reply.text.empty() ? "[empty]" : reply.text));
}

void Conversation::refreshReplies() {
    vector<string> lines;
    if (!_autoPickFirstReply) {
        for (size_t i = 0; i < _replies.size(); ++i) {
            lines.push_back(getReplyText(*_replies[i], static_cast<int>(i)));
        }
    }
    setReplyLines(move(lines));
}

void Conversation::pickReply(int index) {
    debug("Pick reply " + to_string(index), LogChannels::conversation);
    const Dialog::EntryReply &reply = *_replies[index];

    // Run reply script
    if (!reply.script.empty()) {
        _game->scriptRunner().run(reply.script, _owner->id());
    }

    int entryIdx = indexOfFirstActive(reply.entries);
    if (entryIdx == -1) {
        debug("Finish (no active entries)", LogChannels::conversation);
        finish();
        return;
    }
    loadEntry(entryIdx);
}

bool Conversation::handle(const SDL_Event &event) {
    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            if (handleMouseButtonDown(event.button)) return true;
            break;
        case SDL_KEYUP:
            if (handleKeyUp(event.key)) return true;
            break;
        default:
            break;
    }

    return GameGUI::handle(event);
}

bool Conversation::handleMouseButtonDown(const SDL_MouseButtonEvent &event) {
    if (event.button == SDL_BUTTON_LEFT && !_entryEnded && isSkippableEntry()) {
        endCurrentEntry();
        return true;
    }
    return false;
}

bool Conversation::isSkippableEntry() const {
    return g_allEntriesSkippable || (_dialog->isSkippable() && !_paused);
}

void Conversation::endCurrentEntry() {
    _entryEnded = true;

    // Stop voice over, if any
    if (_currentVoice) {
        _currentVoice->stop();
        _currentVoice.reset();
    }

    onEntryEnded();

    if (_autoPickFirstReply) {
        pickReply(0);
    } else if (_replies.empty()) {
        debug("Finish (no active replies", LogChannels::conversation);
        finish();
    }
}

void Conversation::onEntryEnded() {
}

bool Conversation::handleKeyUp(const SDL_KeyboardEvent &event) {
    SDL_Scancode key = event.keysym.scancode;
    if (key >= SDL_SCANCODE_1 && key <= SDL_SCANCODE_9) {
        int index = key - SDL_SCANCODE_1;
        if (_entryEnded) {
            pickReply(index);
            return true;
        }
    }

    return false;
}

void Conversation::update(float dt) {
    GameGUI::update(dt);

    if (!_entryEnded &&
        ((_currentVoice && _currentVoice->isStopped()) || _endEntryTimer.advance(dt))) {

        endCurrentEntry();
    }
}

CameraType Conversation::getCamera(int &cameraId) const {
    string cameraModel(_dialog->cameraModel());
    if (!cameraModel.empty()) {
        return CameraType::Animated;
    }
    if (_currentEntry->cameraId != -1) {
        cameraId = _currentEntry->cameraId;
        return CameraType::Static;
    }
    return CameraType::Dialog;
}

void Conversation::pause() {
    _paused = true;
}

void Conversation::resume() {
    _paused = false;
}

} // namespace game

} // namespace reone
