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

#include "window.h"

#include <stdexcept>

#include <boost/format.hpp>

#include "GL/glew.h"

#include "SDL2/SDL.h"

#include "glm/ext.hpp"

#include "../common/log.h"

#include "cursor.h"
#include "mesh/aabb.h"
#include "mesh/billboard.h"
#include "mesh/cube.h"
#include "mesh/quad.h"
#include "shaders.h"

using namespace std;

namespace reone {

namespace render {

RenderWindow::RenderWindow(const GraphicsOptions &opts, IEventHandler *eventHandler) : _eventHandler(eventHandler), _opts(opts) {
}

void RenderWindow::init() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN;
    if (_opts.fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }
    _window = SDL_CreateWindow(
        "reone",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        _opts.width,
        _opts.height,
        flags);

    if (!_window) {
        throw runtime_error("Failed to create a window: " + string(SDL_GetError()));
    }
    _context = SDL_GL_CreateContext(_window);

    if (!_context) {
        throw runtime_error("Failed to create a GL context: " + string(SDL_GetError()));
    }
    SDL_GL_SetSwapInterval(0);
    glewInit();

    Shaders::instance().initGL();
    CubeMesh::instance().initGL();
    BillboardMesh::instance().initGL();
    AABBMesh::instance().initGL();
    Quad::getDefault().initGL();
    Quad::getXFlipped().initGL();
    Quad::getYFlipped().initGL();
    Quad::getXYFlipped().initGL();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void RenderWindow::deinit() {
    if (_fpsGlobal.hasAverage()) {
        debug("Average FPS: " + to_string(static_cast<int>(_fpsGlobal.average())));
    }
    Quad::getDefault().deinitGL();
    Quad::getXFlipped().deinitGL();
    Quad::getYFlipped().deinitGL();
    Quad::getXYFlipped().deinitGL();
    AABBMesh::instance().deinitGL();
    BillboardMesh::instance().deinitGL();
    CubeMesh::instance().deinitGL();
    Shaders::instance().deinitGL();

    SDL_GL_DeleteContext(_context);
    SDL_DestroyWindow(_window);
    SDL_Quit();
}

void RenderWindow::show() {
    SDL_ShowWindow(_window);
}

void RenderWindow::processEvents(bool &quit) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (handleEvent(event, quit)) continue;
        if (!_eventHandler) continue;

        _eventHandler->handle(event);
    }
}

bool RenderWindow::handleEvent(const SDL_Event &event, bool &quit) {
    switch (event.type) {
        case SDL_QUIT:
            quit = true;
            return true;

        case SDL_KEYDOWN:
            return handleKeyDownEvent(event.key, quit);

        default:
            return false;
    }
}

bool RenderWindow::handleKeyDownEvent(const SDL_KeyboardEvent &event, bool &quit) {
    switch (event.keysym.scancode) {
        case SDL_SCANCODE_C:
            if (event.keysym.mod & KMOD_CTRL) {
                quit = true;
                return true;
            }
            return false;

        default:
            return false;
    }
}

void RenderWindow::update(float dt) {
    _fpsLocal.update(dt);
    _fpsGlobal.update(dt);

    if (_fpsLocal.hasAverage()) {
        SDL_SetWindowTitle(_window, str(boost::format("reone [FPS: %d]") % static_cast<int>(_fpsLocal.average())).c_str());
        _fpsLocal.reset();
    }
}

void RenderWindow::clear() const {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderWindow::drawCursor() const {
    if (_relativeMouseMode) return;

    int x, y;
    uint32_t state = SDL_GetMouseState(&x, &y);
    bool pressed = state & SDL_BUTTON(1);

    _cursor->setPosition({ x, y });
    _cursor->setPressed(pressed);
    _cursor->render();
}

void RenderWindow::swapBuffers() const {
    SDL_GL_SwapWindow(_window);
}

void RenderWindow::setRelativeMouseMode(bool enabled) {
    SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE);
    _relativeMouseMode = enabled;
}

void RenderWindow::setCursor(const shared_ptr<Cursor> &cursor) {
    _cursor = cursor;
    SDL_ShowCursor(!static_cast<bool>(cursor));
}

} // namespace render

} // namespace reone
