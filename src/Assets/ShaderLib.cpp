//=============================================================================
// OpenGLCppWrapper: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenGLCppWrapper is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#include "Assets/ShaderLib.hpp"

#include <string>

namespace assets::shaders
{

namespace
{

constexpr char const* POINT_LIGHT_UNIFORMS = R"(
uniform int pointLightCount;
uniform vec3 pointLightPos0;
uniform vec3 pointLightPos1;
uniform vec3 pointLightPos2;
uniform vec3 pointLightPos3;
uniform vec3 pointLightColor0;
uniform vec3 pointLightColor1;
uniform vec3 pointLightColor2;
uniform vec3 pointLightColor3;
uniform float pointLightRange0;
uniform float pointLightRange1;
uniform float pointLightRange2;
uniform float pointLightRange3;

vec3 samplePointLight(vec3 n, vec3 worldPos, vec3 pos, vec3 col, float range)
{
    vec3 toLight = pos - worldPos;
    float dist = length(toLight);
    if (dist > range)
    {
        return vec3(0.0);
    }
    vec3 L = toLight / dist;
    float att = 1.0 / (1.0 + (dist * dist) / (range * range));
    return max(dot(n, L), 0.0) * att * col;
}

vec3 pointLightsContribution(vec3 n, vec3 worldPos)
{
    vec3 sum = vec3(0.0);
    if (pointLightCount > 0)
    {
        sum += samplePointLight(n, worldPos, pointLightPos0, pointLightColor0, pointLightRange0);
    }
    if (pointLightCount > 1)
    {
        sum += samplePointLight(n, worldPos, pointLightPos1, pointLightColor1, pointLightRange1);
    }
    if (pointLightCount > 2)
    {
        sum += samplePointLight(n, worldPos, pointLightPos2, pointLightColor2, pointLightRange2);
    }
    if (pointLightCount > 3)
    {
        sum += samplePointLight(n, worldPos, pointLightPos3, pointLightColor3, pointLightRange3);
    }
    return sum;
}
)";

constexpr char const* LIT_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vNormal;
out vec3 vWorldPos;

void main()
{
    vec4 world = model * vec4(position, 1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(transpose(inverse(model))) * normal;
    gl_Position = projection * view * world;
}
)";

constexpr char const* LIT_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
in vec3 vWorldPos;

uniform vec3 lightDir;
uniform vec3 ambient;
uniform vec3 color;
)";

constexpr char const* LIT_FRAGMENT_TAIL = R"(

out vec4 oColor;

void main()
{
    vec3 n = normalize(vNormal);
    vec3 toLight = normalize(-lightDir);
    vec3 lit = vec3(max(dot(n, toLight), 0.0)) + pointLightsContribution(n, vWorldPos);
    vec3 shaded = color * (ambient + lit);
    oColor = vec4(pow(shaded, vec3(1.0 / 2.2)), 1.0);
}
)";

constexpr char const* PBR_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;
in vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vNormal;
out vec2 vUv;
out vec3 vWorldPos;

void main()
{
    vec4 world = model * vec4(position, 1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(transpose(inverse(model))) * normal;
    vUv = uv;
    gl_Position = projection * view * world;
}
)";

constexpr char const* PBR_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
in vec2 vUv;
in vec3 vWorldPos;

uniform vec3 lightDir;
uniform vec3 ambient;
uniform vec3 baseColorFactor;
uniform sampler2D baseColorMap;
uniform bool hasBaseColorMap;
)";

constexpr char const* PBR_FRAGMENT_TAIL = R"(

out vec4 oColor;

void main()
{
    vec3 albedo = baseColorFactor;
    if (hasBaseColorMap)
    {
        albedo *= texture(baseColorMap, vUv).rgb;
    }
    vec3 n = normalize(vNormal);
    vec3 toLight = normalize(-lightDir);
    vec3 lit = vec3(max(dot(n, toLight), 0.0)) + pointLightsContribution(n, vWorldPos);
    vec3 shaded = albedo * (ambient + lit);
    oColor = vec4(pow(shaded, vec3(1.0 / 2.2)), 1.0);
}
)";

std::string concat3(char const* a, char const* b, char const* c)
{
    return std::string(a) + b + c;
}

} // namespace

char const* litVertex() { return LIT_VERTEX; }

char const* litFragment()
{
    static std::string const source =
        concat3(LIT_FRAGMENT, POINT_LIGHT_UNIFORMS, LIT_FRAGMENT_TAIL);
    return source.c_str();
}

char const* pbrVertex() { return PBR_VERTEX; }

char const* pbrFragment()
{
    static std::string const source =
        concat3(PBR_FRAGMENT, POINT_LIGHT_UNIFORMS, PBR_FRAGMENT_TAIL);
    return source.c_str();
}

constexpr char const* DEPTH_VERTEX = R"(#version 450 core
in vec3 position;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out float vEyeDepth;
void main()
{
    vec4 world = model * vec4(position, 1.0);
    vec4 eye = view * world;
    vEyeDepth = -eye.z;
    gl_Position = projection * eye;
}
)";

constexpr char const* DEPTH_FRAGMENT = R"(#version 450 core
in float vEyeDepth;
uniform float near;
uniform float far;
uniform float opacity;
out vec4 oColor;
void main()
{
    float shade = 1.0 - smoothstep(near, far, vEyeDepth);
    oColor = vec4(vec3(shade), opacity);
}
)";

constexpr char const* NORMALS_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 vNormal;
void main()
{
    vec4 world = model * vec4(position, 1.0);
    vNormal = mat3(transpose(inverse(view * model))) * normal;
    gl_Position = projection * view * world;
}
)";

constexpr char const* NORMALS_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
uniform float opacity;
out vec4 oColor;
void main()
{
    oColor = vec4(0.5 * normalize(vNormal) + 0.5, opacity);
}
)";

char const* depthVertex() { return DEPTH_VERTEX; }

char const* depthFragment() { return DEPTH_FRAGMENT; }

char const* normalsVertex() { return NORMALS_VERTEX; }

char const* normalsFragment() { return NORMALS_FRAGMENT; }

} // namespace assets::shaders
