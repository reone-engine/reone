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

#pragma once

#include <functional>
#include <memory>

#include "fps.h"
#include "texture.h"
#include "types.h"

namespace reone {

namespace render {

class Cursor;

class RenderWindow {
public:
    RenderWindow(const GraphicsOptions &opts, IEventHandler *eventHandler = nullptr);

    void init();
    void deinit();

    void show();
    void processEvents(bool &quit);
    void update(float dt);

    void setRelativeMouseMode(bool enabled);
    void setCursor(const std::shared_ptr<Cursor> &cursor);

    // Rendering

    void clear() const;
    void drawCursor() const;
    void swapBuffers() const;

    // END Rendering

private:
    GraphicsOptions _opts;
    IEventHandler *_eventHandler { nullptr };
    SDL_Window *_window { nullptr };
    SDL_GLContext _context { nullptr };
    bool _relativeMouseMode { false };
    std::shared_ptr<Cursor> _cursor;
    FpsCounter _fpsLocal;
    FpsCounter _fpsGlobal;
    std::function<void()> _onRenderWorld;
    std::function<void()> _onRenderGUI;

    bool handleEvent(const SDL_Event &event, bool &quit);
    bool handleKeyDownEvent(const SDL_KeyboardEvent &event, bool &quit);
};

} // namespace render

} // namespace reone
