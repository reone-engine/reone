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

#include "actionqueue.h"

#include <algorithm>

#include "SDL2/SDL_timer.h"

using namespace std;

namespace reone {

namespace game {

void ActionQueue::clear() {
    while (!_actions.empty()) {
        _actions.pop_front();
    }
}

void ActionQueue::add(unique_ptr<Action> action) {
    _actions.push_back(move(action));
}

void ActionQueue::delay(unique_ptr<Action> action, float seconds) {
    DelayedAction delayed;
    delayed.action = move(action);
    delayed.timestamp = SDL_GetTicks() + static_cast<int>(1000.0f * seconds);
    _delayed.push_back(move(delayed));
}

void ActionQueue::update() {
    removeCompletedActions();
    updateDelayedActions();
}

ActionQueue::iterator ActionQueue::begin() {
    return _actions.begin();
}

ActionQueue::iterator ActionQueue::end() {
    return _actions.end();
}

void ActionQueue::removeCompletedActions() {
    while (true) {
        shared_ptr<Action> action(currentAction());
        if (!action || !action->isCompleted()) return;

        _actions.pop_front();
    }
}

void ActionQueue::updateDelayedActions() {
    uint32_t now = SDL_GetTicks();

    for (auto &delayed : _delayed) {
        if (now >= delayed.timestamp) {
            _actions.push_back(move(delayed.action));
        }
    }
    auto delayedToRemove = remove_if(
        _delayed.begin(),
        _delayed.end(),
        [&now](const DelayedAction &delayed) { return now >= delayed.timestamp; });

    _delayed.erase(delayedToRemove, _delayed.end());
}

bool ActionQueue::empty() const {
    return _actions.empty();
}

int ActionQueue::size() const {
    return static_cast<int>(_actions.size());
}

shared_ptr<Action> ActionQueue::currentAction() {
   return _actions.empty() ? nullptr : _actions.front();
}

} // namespace game

} // namespace reone
