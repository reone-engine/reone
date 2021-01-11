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

#include "shaders.h"

#include <cstdarg>
#include <stdexcept>

#include <boost/format.hpp>

#include "GL/glew.h"

#include "SDL2/SDL_opengl.h"

#include "glm/ext.hpp"

using namespace std;

namespace reone {

namespace render {

static const int kFeaturesBindingPointIndex = 1;
static const int kGeneralBindingPointIndex = 2;
static const int kLightingBindingPointIndex = 3;
static const int kSkeletalBindingPointIndex = 4;

static const GLchar kCommonShaderHeader[] = R"END(
#version 330

const int MAX_LIGHTS = 8;
const int MAX_BONES = 128;
const float SHADOW_FAR_PLANE = 10000.0;

struct Light {
    vec4 position;
    vec4 color;
    float radius;
};

layout(std140) uniform General {
    bool uLightmapEnabled;
    bool uEnvmapEnabled;
    bool uBumpyShinyEnabled;
    bool uBumpmapEnabled;
    bool uSkeletalEnabled;
    bool uLightingEnabled;
    bool uSelfIllumEnabled;
    bool uBlurEnabled;
    bool uBloomEnabled;
    bool uDiscardEnabled;
    bool uShadowsEnabled;

    uniform mat4 uModel;
    uniform vec4 uColor;
    uniform float uAlpha;
    uniform vec4 uSelfIllumColor;
    uniform vec4 uDiscardColor;
    uniform vec2 uBlurResolution;
    uniform vec2 uBlurDirection;
    uniform vec2 uBillboardGridSize;
    uniform vec2 uBillboardSize;
    uniform vec4 uParticleCenter;
    uniform int uBillboardFrame;
    uniform bool uBillboardToWorldZ;
};

layout(std140) uniform Lighting {
    vec4 uAmbientLightColor;
    int uLightCount;
    Light uLights[MAX_LIGHTS];
};

layout(std140) uniform Skeletal {
    uniform mat4 uAbsTransform;
    uniform mat4 uAbsTransformInv;
    uniform mat4 uBones[MAX_BONES];
};
)END";

static const GLchar kSourceVertexGUI[] = R"END(
uniform mat4 uProjection;
uniform mat4 uView;

layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 aTexCoords;

out vec2 fragTexCoords;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
    fragTexCoords = aTexCoords;
}
)END";

static const GLchar kSourceVertexModel[] = R"END(
uniform mat4 uProjection;
uniform mat4 uView;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec2 aLightmapCoords;
layout(location = 4) in vec4 aBoneWeights;
layout(location = 5) in vec4 aBoneIndices;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoords;
out vec2 fragLightmapCoords;

void main() {
    vec3 newPosition = vec3(0.0);

    if (uSkeletalEnabled) {
        float weight0 = aBoneWeights.x;
        float weight1 = aBoneWeights.y;
        float weight2 = aBoneWeights.z;
        float weight3 = aBoneWeights.w;

        int index0 = int(aBoneIndices.x);
        int index1 = int(aBoneIndices.y);
        int index2 = int(aBoneIndices.z);
        int index3 = int(aBoneIndices.w);

        vec4 position4 = vec4(aPosition, 1.0);

        newPosition += weight0 * (uAbsTransformInv * uBones[index0] * uAbsTransform * position4).xyz;
        newPosition += weight1 * (uAbsTransformInv * uBones[index1] * uAbsTransform * position4).xyz;
        newPosition += weight2 * (uAbsTransformInv * uBones[index2] * uAbsTransform * position4).xyz;
        newPosition += weight3 * (uAbsTransformInv * uBones[index3] * uAbsTransform * position4).xyz;

    } else {
        newPosition = aPosition;
    }
    vec4 newPosition4 = vec4(newPosition, 1.0);

    gl_Position = uProjection * uView * uModel * newPosition4;
    fragPosition = vec3(uModel * newPosition4);
    fragNormal = mat3(transpose(inverse(uModel))) * aNormal;
    fragTexCoords = aTexCoords;
    fragLightmapCoords = aLightmapCoords;
}
)END";

static const GLchar kSourceVertexDepth[] = R"END(
layout(location = 0) in vec3 aPosition;

void main() {
    gl_Position = uModel * vec4(aPosition, 1.0);
}
)END";

static const GLchar kSourceVertexBillboard[] = R"END(
const vec3 RIGHT = vec3(1.0, 0.0, 0.0);
const vec3 FORWARD = vec3(0.0, 1.0, 0.0);

uniform mat4 uProjection;
uniform mat4 uView;

layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 aTexCoords;

out vec2 fragTexCoords;

void main() {
    vec3 position;

    if (uBillboardToWorldZ) {
        position = 
            uParticleCenter.xyz +
            RIGHT * aPosition.x * uBillboardSize.x +
            FORWARD * aPosition.y * uBillboardSize.y;

    } else {
        vec3 cameraRight = vec3(uView[0][0], uView[1][0], uView[2][0]);
        vec3 cameraUp = vec3(uView[0][1], uView[1][1], uView[2][1]);

        position =
            uParticleCenter.xyz +
            cameraRight * aPosition.x * uBillboardSize.x +
            cameraUp * aPosition.y * uBillboardSize.y;
    }

    gl_Position = uProjection * uView * vec4(position, 1.0);
    fragTexCoords = aTexCoords;
}
)END";

static const GLchar kSourceGeometryDepth[] = R"END(
const int NUM_CUBE_FACES = 6;

layout(triangles) in;
layout(triangle_strip, max_vertices=18) out;

uniform mat4 uShadowMatrices[NUM_CUBE_FACES];

out vec4 fragPosition;

void main() {
    for (int face = 0; face < NUM_CUBE_FACES; ++face) {
        gl_Layer = face;
        for (int i = 0; i < 3; ++i) {
            fragPosition = gl_in[i].gl_Position;
            gl_Position = uShadowMatrices[face] * fragPosition;
            EmitVertex();
        }
        EndPrimitive();
    }
}
)END";

static const GLchar kSourceFragmentWhite[] = R"END(
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragColorBright;

void main() {
    fragColor = vec4(uColor.rgb, uAlpha);
    fragColorBright = vec4(0.0, 0.0, 0.0, 1.0);
}
)END";

static const GLchar kSourceFragmentGUI[] = R"END(
uniform sampler2D uTexture;

in vec2 fragTexCoords;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragColorBright;

void main() {
    vec4 textureSample = texture(uTexture, fragTexCoords);
    vec3 finalColor = uColor.rgb * textureSample.rgb;

    if (uDiscardEnabled && length(uDiscardColor.rgb - finalColor) < 0.01) {
        discard;
    }
    fragColor = vec4(finalColor, uAlpha * textureSample.a);
    fragColorBright = vec4(0.0, 0.0, 0.0, 1.0);
}
)END";

static const GLchar kSourceFragmentModel[] = R"END(
const vec3 RGB_TO_LUMINOSITY = vec3(0.2126, 0.7152, 0.0722);

uniform sampler2D uDiffuse;
uniform sampler2D uLightmap;
uniform sampler2D uBumpmap;
uniform samplerCube uEnvmap;
uniform samplerCube uBumpyShiny;
uniform samplerCube uShadowmap;

uniform vec3 uCameraPosition;
uniform bool uShadowLightPresent;
uniform vec3 uShadowLightPosition;

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoords;
in vec2 fragLightmapCoords;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 fragColorBright;

void applyLightmap(inout vec3 color) {
    vec4 lightmapSample = texture(uLightmap, fragLightmapCoords);
    color *= lightmapSample.rgb;
}

void applyEnvmap(samplerCube image, vec3 normal, float a, inout vec3 color) {
    vec3 I = normalize(fragPosition - uCameraPosition);
    vec3 R = reflect(I, normal);
    vec4 sample = texture(image, R);

    color += sample.rgb * a;
}

void applyLighting(vec3 normal, inout vec3 color) {
    color += uAmbientLightColor.rgb;

    if (uLightCount == 0) return;

    for (int i = 0; i < uLightCount; ++i) {
        vec3 surfaceToLight = uLights[i].position.xyz - fragPosition;
        vec3 lightDir = normalize(surfaceToLight);

        vec3 surfaceToCamera = normalize(uCameraPosition - fragPosition);
        float diffuse = max(dot(normal, lightDir), 0.0);
        float specular = 0.0;

        if (diffuse > 0.0) {
            specular = 0.25 * pow(max(0.0, dot(surfaceToCamera, reflect(-lightDir, normal))), 32);
        }
        float distToLight = length(surfaceToLight);

        float attenuation = clamp(1.0 - distToLight / uLights[i].radius, 0.0, 1.0);
        attenuation *= attenuation;

        color += attenuation * (diffuse + specular) * uLights[i].color.rgb;
    }
}

float getShadowValue() {
    if (!uShadowLightPresent) return 0.0;

    vec3 fragToLight = fragPosition - uShadowLightPosition.xyz;
    float currentDepth = length(fragToLight);

    float shadow = 0.0;
    float bias = 0.05;
    float samples = 4.0;
    float offset = 0.1;

    for (float x = -offset; x < offset; x += offset / (samples * 0.5)) {
        for (float y = -offset; y < offset; y += offset / (samples * 0.5)) {
            for (float z = -offset; z < offset; z += offset / (samples * 0.5)) {
                float closestDepth = texture(uShadowmap, fragToLight + vec3(x, y, z)).r;
                closestDepth *= SHADOW_FAR_PLANE;

                if (currentDepth - bias > closestDepth) {
                    shadow += 1.0;
                }
            }
        }
    }

    return shadow / (samples * samples * samples);
}

void main() {
    vec4 diffuseSample = texture(uDiffuse, fragTexCoords);
    vec3 surfaceColor = diffuseSample.rgb;
    float shadow = 0.0f;
    vec3 lightColor = vec3(0.0);
    vec3 normal = normalize(fragNormal);

    if (uLightmapEnabled) {
        applyLightmap(surfaceColor);
    }
    if (uEnvmapEnabled) {
        applyEnvmap(uEnvmap, normal, 1.0 - diffuseSample.a, surfaceColor);
    } else if (uBumpyShinyEnabled) {
        applyEnvmap(uBumpyShiny, normal, 1.0 - diffuseSample.a, surfaceColor);
    }
    if (uLightingEnabled) {
        applyLighting(normal, lightColor);
        lightColor = min(lightColor, 1.0);
    } else {
        lightColor = vec3(1.0);
    }
    if (uShadowsEnabled) {
        lightColor *= (1.0 - 0.5 * getShadowValue());
    }
    float finalAlpha = uAlpha;

    if (!uEnvmapEnabled && !uBumpyShinyEnabled && !uBumpmapEnabled) {
        finalAlpha *= diffuseSample.a;
    }
    fragColor = vec4(lightColor * surfaceColor, finalAlpha);

    if (uSelfIllumEnabled) {
        vec3 color = uSelfIllumColor.rgb * diffuseSample.rgb;
        fragColorBright = vec4(smoothstep(0.75, 1.0, color), 1.0);
    } else {
        fragColorBright = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
)END";

static const GLchar kSourceFragmentBlur[] = R"END(
uniform sampler2D uTexture;

out vec4 fragColor;

void main() {
    vec2 uv = vec2(gl_FragCoord.xy / uBlurResolution);
    vec4 color = vec4(0.0);
    vec2 off1 = vec2(1.3846153846) * uBlurDirection;
    vec2 off2 = vec2(3.2307692308) * uBlurDirection;
    color += texture2D(uTexture, uv) * 0.2270270270;
    color += texture2D(uTexture, uv + (off1 / uBlurResolution)) * 0.3162162162;
    color += texture2D(uTexture, uv - (off1 / uBlurResolution)) * 0.3162162162;
    color += texture2D(uTexture, uv + (off2 / uBlurResolution)) * 0.0702702703;
    color += texture2D(uTexture, uv - (off2 / uBlurResolution)) * 0.0702702703;

    fragColor = color;
}
)END";

static const GLchar kSourceFragmentBloom[] = R"END(
uniform sampler2D uGeometry;
uniform sampler2D uBloom;

in vec2 fragTexCoords;

out vec4 fragColor;

void main() {
    vec3 geometryColor = texture(uGeometry, fragTexCoords).rgb;
    vec3 bloomColor = texture(uBloom, fragTexCoords).rgb;

    fragColor = vec4(geometryColor + bloomColor, 1.0);
}
)END";

static const GLchar kSourceFragmentDepth[] = R"END(
uniform vec3 uShadowLightPosition;

in vec4 fragPosition;

void main() {
    float lightDistance = length(fragPosition.xyz - uShadowLightPosition);
    lightDistance = lightDistance / SHADOW_FAR_PLANE; // map to [0,1]
    gl_FragDepth = lightDistance;
}
)END";

static const GLchar kSourceFragmentBillboard[] = R"END(
uniform sampler2D uTexture;

in vec2 fragTexCoords;

out vec4 fragColor;

void main() {
    float oneOverGridX = 1.0 / uBillboardGridSize.x;
    float oneOverGridY = 1.0 / uBillboardGridSize.y;

    vec2 texCoords = fragTexCoords;
    texCoords.x *= oneOverGridX;
    texCoords.y *= oneOverGridY;

    if (uBillboardFrame > 0) {
        texCoords.y += oneOverGridY * (uBillboardFrame / int(uBillboardGridSize.x));
        texCoords.x += oneOverGridX * (uBillboardFrame % int(uBillboardGridSize.x));
    }

    vec4 textureSample = texture(uTexture, texCoords);
    fragColor = vec4(uColor.rgb * textureSample.rgb, uAlpha * textureSample.a);
}
)END";

static const GLchar kSourceFragmentDebugShadows[] = R"END(
uniform samplerCube uShadowmap;

in vec2 fragTexCoords;

out vec4 fragColor;

void main() {
    vec2 cubeMapCoords = 2.0 * fragTexCoords - 1.0;
    float value = texture(uShadowmap, vec3(cubeMapCoords, -1.0)).r;
    fragColor = vec4(vec3(value), 1.0);
}
)END";

Shaders &Shaders::instance() {
    static Shaders instance;
    return instance;
}

Shaders::Shaders() {
    _lightingUniforms = make_shared<LightingUniforms>();
    _skeletalUniforms = make_shared<SkeletalUniforms>();
}

void Shaders::initGL() {
    initShader(ShaderName::VertexGUI, GL_VERTEX_SHADER, kSourceVertexGUI);
    initShader(ShaderName::VertexModel, GL_VERTEX_SHADER, kSourceVertexModel);
    initShader(ShaderName::VertexDepth, GL_VERTEX_SHADER, kSourceVertexDepth);
    initShader(ShaderName::VertexBillboard, GL_VERTEX_SHADER, kSourceVertexBillboard);
    initShader(ShaderName::GeometryDepth, GL_GEOMETRY_SHADER, kSourceGeometryDepth);
    initShader(ShaderName::FragmentWhite, GL_FRAGMENT_SHADER, kSourceFragmentWhite);
    initShader(ShaderName::FragmentGUI, GL_FRAGMENT_SHADER, kSourceFragmentGUI);
    initShader(ShaderName::FragmentModel, GL_FRAGMENT_SHADER, kSourceFragmentModel);
    initShader(ShaderName::FragmentBlur, GL_FRAGMENT_SHADER, kSourceFragmentBlur);
    initShader(ShaderName::FragmentBloom, GL_FRAGMENT_SHADER, kSourceFragmentBloom);
    initShader(ShaderName::FragmentDepth, GL_FRAGMENT_SHADER, kSourceFragmentDepth);
    initShader(ShaderName::FragmentBillboard, GL_FRAGMENT_SHADER, kSourceFragmentBillboard);
    initShader(ShaderName::FragmentDebugShadows, GL_FRAGMENT_SHADER, kSourceFragmentDebugShadows);

    initProgram(ShaderProgram::GUIGUI, 2, ShaderName::VertexGUI, ShaderName::FragmentGUI);
    initProgram(ShaderProgram::GUIBlur, 2, ShaderName::VertexGUI, ShaderName::FragmentBlur);
    initProgram(ShaderProgram::GUIBloom, 2, ShaderName::VertexGUI, ShaderName::FragmentBloom);
    initProgram(ShaderProgram::GUIWhite, 2, ShaderName::VertexGUI, ShaderName::FragmentWhite);
    initProgram(ShaderProgram::GUIDebugShadows, 2, ShaderName::VertexGUI, ShaderName::FragmentDebugShadows);
    initProgram(ShaderProgram::ModelWhite, 2, ShaderName::VertexModel, ShaderName::FragmentWhite);
    initProgram(ShaderProgram::ModelModel, 2, ShaderName::VertexModel, ShaderName::FragmentModel);
    initProgram(ShaderProgram::BillboardBillboard, 2, ShaderName::VertexBillboard, ShaderName::FragmentBillboard);
    initProgram(ShaderProgram::DepthDepth, 3, ShaderName::VertexDepth, ShaderName::GeometryDepth, ShaderName::FragmentDepth);

    glGenBuffers(1, &_generalUbo);
    glGenBuffers(1, &_lightingUbo);
    glGenBuffers(1, &_skeletalUbo);

    for (auto &program : _programs) {
        glUseProgram(program.second);
        _activeOrdinal = program.second;

        uint32_t generalBlockIdx = glGetUniformBlockIndex(_activeOrdinal, "General");
        if (generalBlockIdx != GL_INVALID_INDEX) {
            glUniformBlockBinding(_activeOrdinal, generalBlockIdx, kGeneralBindingPointIndex);
            //GLint blockSize;
            //glGetActiveUniformBlockiv(_activeOrdinal, generalBlockIdx, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
        }
        uint32_t lightingBlockIdx = glGetUniformBlockIndex(_activeOrdinal, "Lighting");
        if (lightingBlockIdx != GL_INVALID_INDEX) {
            glUniformBlockBinding(_activeOrdinal, lightingBlockIdx, kLightingBindingPointIndex);
        }
        uint32_t skeletalBlockIdx = glGetUniformBlockIndex(_activeOrdinal, "Skeletal");
        if (skeletalBlockIdx != GL_INVALID_INDEX) {
            glUniformBlockBinding(_activeOrdinal, skeletalBlockIdx, kSkeletalBindingPointIndex);
        }

        setUniform("uEnvmap", TextureUnits::envmap);
        setUniform("uLightmap", TextureUnits::lightmap);
        setUniform("uBumpyShiny", TextureUnits::bumpyShiny);
        setUniform("uBumpmap", TextureUnits::bumpmap);
        setUniform("uBloom", TextureUnits::bloom);
        setUniform("uShadowmap", TextureUnits::shadowmap);

        _activeOrdinal = 0;
        glUseProgram(0);
    }
}

void Shaders::initShader(ShaderName name, unsigned int type, const char *source) {
    GLuint shader = glCreateShader(type);
    GLint success;
    char log[512];
    GLsizei logSize;

    const GLchar *sources[] = { kCommonShaderHeader, source };
    glShaderSource(shader, 2, sources, nullptr);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        glGetShaderInfoLog(shader, sizeof(log), &logSize, log);
        throw runtime_error(str(boost::format("Shader %d compilation failed: %s") % static_cast<int>(name) % string(log, logSize)));
    }

    _shaders.insert(make_pair(name, shader));
}

void Shaders::initProgram(ShaderProgram program, int shaderCount, ...) {
    GLuint ordinal = glCreateProgram();

    va_list args;
    va_start(args, shaderCount);
    for (int i = 0; i < shaderCount; ++i) {
        ShaderName name = va_arg(args, ShaderName);
        unsigned int shaderOrdinal = _shaders.find(name)->second;
        glAttachShader(ordinal, shaderOrdinal);
    }
    va_end(args);

    glLinkProgram(ordinal);

    GLint success;
    char log[512];
    GLsizei logSize;

    glGetProgramiv(ordinal, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(ordinal, sizeof(log), &logSize, log);
        throw runtime_error("Shaders: program linking failed: " + string(log, logSize));
    }

    _programs.insert(make_pair(program, ordinal));
}

Shaders::~Shaders() {
    deinitGL();
}

void Shaders::deinitGL() {
    if (_skeletalUbo) {
        glDeleteBuffers(1, &_skeletalUbo);
        _skeletalUbo = 0;
    }
    if (_lightingUbo) {
        glDeleteBuffers(1, &_lightingUbo);
        _lightingUbo = 0;
    }
    if (_generalUbo) {
        glDeleteBuffers(1, &_generalUbo);
        _generalUbo = 0;
    }
    for (auto &pair :_programs) {
        glDeleteProgram(pair.second);
    }
    _programs.clear();

    for (auto &pair : _shaders) {
        glDeleteShader(pair.second);
    }
    _shaders.clear();
}

void Shaders::activate(ShaderProgram program, const LocalUniforms &locals) {
    if (_activeProgram != program) {
        unsigned int ordinal = getOrdinal(program);
        glUseProgram(ordinal);

        _activeProgram = program;
        _activeOrdinal = ordinal;
    }
    setLocalUniforms(locals);
}

unsigned int Shaders::getOrdinal(ShaderProgram program) const {
    auto it = _programs.find(program);
    if (it == _programs.end()) {
        throw invalid_argument("Shaders: program not found: " + to_string(static_cast<int>(program)));
    }
    return it->second;
}

void Shaders::setLocalUniforms(const LocalUniforms &locals) {
    glBindBufferBase(GL_UNIFORM_BUFFER, kGeneralBindingPointIndex, _generalUbo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GeneralUniforms), &locals.general, GL_STATIC_DRAW);

    if (locals.general.skeletalEnabled) {
        glBindBufferBase(GL_UNIFORM_BUFFER, kSkeletalBindingPointIndex, _skeletalUbo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(SkeletalUniforms), locals.skeletal.get(), GL_STATIC_DRAW);
    }
    if (locals.general.lightingEnabled) {
        glBindBufferBase(GL_UNIFORM_BUFFER, kLightingBindingPointIndex, _lightingUbo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(LightingUniforms), locals.lighting.get(), GL_STATIC_DRAW);
    }
}

void Shaders::setUniform(const string &name, const glm::mat4 &m) {
    setUniform(name, [this, &m](int loc) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(m));
    });
}

void Shaders::setUniform(const string &name, const function<void(int)> &setter) {
    static unordered_map<uint32_t, unordered_map<string, GLint>> locsByProgram;

    unordered_map<string, GLint> &locs = locsByProgram[_activeOrdinal];
    auto maybeLoc = locs.find(name);
    GLint loc = 0;

    if (maybeLoc != locs.end()) {
        loc = maybeLoc->second;
    } else {
        loc = glGetUniformLocation(_activeOrdinal, name.c_str());
        locs.insert(make_pair(name, loc));
    }
    if (loc != -1) {
        setter(loc);
    }
}

void Shaders::setUniform(const string &name, int value) {
    setUniform(name, [this, &value](int loc) {
        glUniform1i(loc, value);
    });
}

void Shaders::setUniform(const string &name, float value) {
    setUniform(name, [this, &value](int loc) {
        glUniform1f(loc, value);
    });
}

void Shaders::setUniform(const string &name, const glm::vec2 &v) {
    setUniform(name, [this, &v](int loc) {
        glUniform2f(loc, v.x, v.y);
    });
}

void Shaders::setUniform(const string &name, const glm::vec3 &v) {
    setUniform(name, [this, &v](int loc) {
        glUniform3f(loc, v.x, v.y, v.z);
    });
}

void Shaders::setUniform(const string &name, const vector<glm::mat4> &arr) {
    setUniform(name, [this, &arr](int loc) {
        glUniformMatrix4fv(loc, static_cast<GLsizei>(arr.size()), GL_FALSE, reinterpret_cast<const GLfloat *>(&arr[0]));
    });
}

void Shaders::deactivate() {
    if (_activeProgram == ShaderProgram::None) return;

    glUseProgram(0);
    _activeProgram = ShaderProgram::None;
    _activeOrdinal = 0;
}

shared_ptr<LightingUniforms> Shaders::lightingUniforms() const {
    return _lightingUniforms;
}

shared_ptr<SkeletalUniforms> Shaders::skeletalUniforms() const {
    return _skeletalUniforms;
}

void Shaders::setGlobalUniforms(const GlobalUniforms &globals) {
    uint32_t ordinal = _activeOrdinal;

    for (auto &program : _programs) {
        glUseProgram(program.second);
        _activeOrdinal = program.second;

        setUniform("uProjection", globals.projection);
        setUniform("uView", globals.view);
        setUniform("uCameraPosition", globals.cameraPosition);
        setUniform("uShadowLightPresent", globals.shadowLightPresent);

        if (globals.shadowLightPresent) {
            setUniform("uShadowLightPosition", globals.shadowLightPosition);

            for (int i = 0; i < kNumCubeFaces; ++i) {
                setUniform(str(boost::format("uShadowMatrices[%d]") % i), globals.shadowMatrices[i]);
            }
        }
    }
    glUseProgram(ordinal);
    _activeOrdinal = ordinal;
}

} // namespace render

} // namespace reone
