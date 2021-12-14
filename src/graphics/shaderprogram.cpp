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

#include "shaderprogram.h"

#include "types.h"

using namespace std;

namespace reone {

namespace graphics {

void ShaderProgram::init() {
    if (_inited) {
        return;
    }
    _nameGL = glCreateProgram();
    for (auto &shader : _shaders) {
        glAttachShader(_nameGL, shader->nameGL());
    }
    glLinkProgram(_nameGL);

    GLint success;
    glGetProgramiv(_nameGL, GL_LINK_STATUS, &success);

    char log[512];
    GLsizei logSize;
    if (!success) {
        glGetProgramInfoLog(_nameGL, sizeof(log), &logSize, log);
        throw runtime_error("Failed linking shader program: " + string(log, logSize));
    }

    _inited = true;
}

void ShaderProgram::deinit() {
    if (!_inited) {
        return;
    }
    glDeleteProgram(_nameGL);
    _shaders.clear();
    _inited = false;
}

void ShaderProgram::use() {
    glUseProgram(_nameGL);
}

void ShaderProgram::bindUniformBlock(const string &name, int bindingPoint) {
    GLuint blockIdx = glGetUniformBlockIndex(_nameGL, name.c_str());
    if (blockIdx == GL_INVALID_INDEX) {
        return;
    }
    glUniformBlockBinding(_nameGL, blockIdx, bindingPoint);
}

void ShaderProgram::setUniform(const string &name, int value) {
    setUniform(name, [this, &value](int loc) {
        glUniform1i(loc, value);
    });
}

void ShaderProgram::setUniform(const string &name, float value) {
    setUniform(name, [this, &value](int loc) {
        glUniform1f(loc, value);
    });
}

void ShaderProgram::setUniform(const string &name, const glm::vec2 &v) {
    setUniform(name, [this, &v](int loc) {
        glUniform2f(loc, v.x, v.y);
    });
}

void ShaderProgram::setUniform(const string &name, const glm::vec3 &v) {
    setUniform(name, [this, &v](int loc) {
        glUniform3f(loc, v.x, v.y, v.z);
    });
}

void ShaderProgram::setUniform(const string &name, const glm::mat4 &m) {
    setUniform(name, [this, &m](int loc) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
    });
}

void ShaderProgram::setUniform(const string &name, const vector<glm::mat4> &arr) {
    setUniform(name, [this, &arr](int loc) {
        glUniformMatrix4fv(loc, static_cast<GLsizei>(arr.size()), GL_FALSE, reinterpret_cast<const GLfloat *>(&arr[0]));
    });
}

void ShaderProgram::setUniform(const string &name, const function<void(int)> &setter) {
    GLint location;

    auto maybeLocation = _uniformLocations.find(name);
    if (maybeLocation != _uniformLocations.end()) {
        location = maybeLocation->second;
    } else {
        location = glGetUniformLocation(_nameGL, name.c_str());
        _uniformLocations.insert(make_pair(name, location));
    }

    if (location != -1) {
        setter(location);
    }
}

} // namespace graphics

} // namespace reone
