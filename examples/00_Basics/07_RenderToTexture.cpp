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

#include "00_Basics/07_RenderToTexture.hpp"

#include "Math/Transformation.hpp"

#include <array>
#include <cmath>
#include <vector>

using namespace units::literals;

namespace examples
{

namespace
{

constexpr std::uint32_t TARGET = 512u;
constexpr std::uint32_t UNIT = 0u;
constexpr float H = 0.6f;

constexpr const char* SCENE_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;
in vec3 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vNormal;
out vec3 vColor;

void main()
{
    vNormal = mat3(model) * normal;
    vColor = color;
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)";

constexpr const char* SCENE_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
in vec3 vColor;
out vec4 oColor;

void main()
{
    const vec3 toLight = normalize(vec3(0.4, 0.8, 0.6));
    float lit = 0.25 + (0.75 * max(dot(normalize(vNormal), toLight), 0.0));
    oColor = vec4(vColor * lit, 1.0);
}
)";

//! \brief A triangle covering the target, built from the vertex index. The
//! texture coordinate is the clip position mapped into [0, 1], so sampling the
//! offscreen picture stretches it over the pass.
constexpr const char* SCREEN_VERTEX = R"(#version 450 core
out vec2 vUV;

void main()
{
    vec2 corners[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    vec2 position = corners[gl_VertexID];
    vUV = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr const char* BLIT_FRAGMENT = R"(#version 450 core
in vec2 vUV;
uniform sampler2D image;
out vec4 oColor;

void main()
{
    oColor = texture(image, vUV);
}
)";

//! \brief The same picture, read three times a few pixels apart. That is what
//! makes the second pass a treatment rather than a copy: the cube is never
//! drawn here, only the texture it left behind.
constexpr const char* PROCESS_FRAGMENT = R"(#version 450 core
in vec2 vUV;
uniform sampler2D image;
out vec4 oColor;

void main()
{
    vec2 shift = vec2(0.008, 0.0);
    float red = texture(image, vUV + shift).r;
    float green = texture(image, vUV).g;
    float blue = texture(image, vUV - shift).b;
    oColor = vec4(red, green, blue, 1.0);
}
)";

} // namespace

//------------------------------------------------------------------------------
std::string RenderToTexture::description() const
{
    return "The cube is drawn into a framebuffer, never into the window. The "
           "left half shows that picture as it is; the right half shows the "
           "same texture through a fullscreen effect. The offscreen target is "
           "512 pixels on a side whatever the window does, and destroying the "
           "framebuffer would not destroy the textures: they are named, not "
           "owned.";
}

//------------------------------------------------------------------------------
gpu::Status RenderToTexture::makeScene()
{
    GPU_TRY_ASSIGN(program,
                   gpu::Program::fromSources(SCENE_VERTEX, SCENE_FRAGMENT));
    m_scene_program = std::move(program);

    std::vector<Vertex> corners;
    std::vector<std::uint16_t> indices;
    corners.reserve(24u);
    indices.reserve(36u);

    struct Face
    {
        Vector3f normal;
        Vector3f color;
    };

    const std::array<Face, 6u> faces{
        Face{ Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.9f, 0.3f, 0.3f) },
        Face{ Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.3f, 0.9f, 0.4f) },
        Face{ Vector3f(1.0f, 0.0f, 0.0f), Vector3f(0.3f, 0.5f, 0.9f) },
        Face{ Vector3f(-1.0f, 0.0f, 0.0f), Vector3f(0.9f, 0.8f, 0.3f) },
        Face{ Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.8f, 0.4f, 0.9f) },
        Face{ Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.4f, 0.9f, 0.9f) }
    };

    for (Face const& face : faces)
    {
        const Vector3f up = (std::abs(face.normal.y) > 0.5f)
                                ? Vector3f(0.0f, 0.0f, 1.0f)
                                : Vector3f(0.0f, 1.0f, 0.0f);
        const Vector3f right = vector::cross(up, face.normal);
        const Vector3f top = vector::cross(face.normal, right);
        const auto first = static_cast<std::uint16_t>(corners.size());

        corners.push_back(
            Vertex{ (face.normal - right - top) * H, face.normal, face.color });
        corners.push_back(
            Vertex{ (face.normal + right - top) * H, face.normal, face.color });
        corners.push_back(
            Vertex{ (face.normal + right + top) * H, face.normal, face.color });
        corners.push_back(
            Vertex{ (face.normal - right + top) * H, face.normal, face.color });

        indices.push_back(first);
        indices.push_back(static_cast<std::uint16_t>(first + 1u));
        indices.push_back(static_cast<std::uint16_t>(first + 2u));
        indices.push_back(first);
        indices.push_back(static_cast<std::uint16_t>(first + 2u));
        indices.push_back(static_cast<std::uint16_t>(first + 3u));
    }

    GPU_TRY_ASSIGN(vertices,
                   gpu::Buffer<Vertex>::from(std::span<const Vertex>(corners),
                                             gpu::BufferKind::Vertex,
                                             gpu::BufferUsage::Immutable));
    m_vertices = std::move(vertices);

    GPU_TRY_ASSIGN(index_buffer,
                   gpu::Buffer<std::uint16_t>::from(
                       std::span<const std::uint16_t>(indices),
                       gpu::BufferKind::Index,
                       gpu::BufferUsage::Immutable));
    m_indices = std::move(index_buffer);

    gpu::RenderState state;
    state.depth_test = true;
    state.cull = gpu::CullMode::Back;

    const gpu::VertexLayout layout =
        GPU_LAYOUT(Vertex, position, normal, color);
    GPU_TRY_ASSIGN(pipeline,
                   gpu::Pipeline::create<Vertex>(m_scene_program, layout, state));
    m_scene = std::move(pipeline);
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status RenderToTexture::makeTarget()
{
    gpu::TextureDesc color;
    color.kind = gpu::TextureKind::Texture2D;
    color.format = gpu::PixelFormat::RGBA8;
    color.width = TARGET;
    color.height = TARGET;
    color.levels = 1u;
    GPU_TRY_ASSIGN(made_color, gpu::Texture::create(color));
    m_color = std::move(made_color);

    gpu::TextureDesc depth;
    depth.kind = gpu::TextureKind::Texture2D;
    depth.format = gpu::PixelFormat::Depth32F;
    depth.width = TARGET;
    depth.height = TARGET;
    depth.levels = 1u;
    depth.magnify = gpu::Filter::Nearest;
    depth.minify = gpu::Filter::Nearest;
    GPU_TRY_ASSIGN(made_depth, gpu::Texture::create(depth));
    m_depth = std::move(made_depth);

    GPU_TRY_ASSIGN(target, gpu::Framebuffer::create(m_color, m_depth));
    m_target = std::move(target);
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status RenderToTexture::makeScreen(gpu::Program& p_program,
                                        gpu::Pipeline& p_pipeline,
                                        char const* p_fragment)
{
    GPU_TRY_ASSIGN(program, gpu::Program::fromSources(SCREEN_VERTEX, p_fragment));
    p_program = std::move(program);
    GPU_TRY(p_program.set("image", static_cast<int>(UNIT)));

    GPU_TRY_ASSIGN(pipeline,
                   gpu::Pipeline::create(p_program, gpu::VertexLayout{}));
    p_pipeline = std::move(pipeline);
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status RenderToTexture::setUp()
{
    GPU_TRY(makeScene());
    GPU_TRY(makeTarget());
    GPU_TRY(makeScreen(m_blit_program, m_blit, BLIT_FRAGMENT));
    GPU_TRY(makeScreen(m_process_program, m_process, PROCESS_FRAGMENT));
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status RenderToTexture::draw(Frame const& p_frame)
{
    const Matrix44f identity(matrix::Identity);
    const Matrix44f model = matrix::rotate(
        identity,
        units::angle::radian_t(p_frame.total * 0.7f),
        Vector3f(0.3f, 1.0f, 0.2f));
    const Matrix44f view = matrix::lookAt(Vector3f(0.0f, 0.0f, 3.0f),
                                          Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 1.0f, 0.0f));
    // The aspect of the offscreen picture, not of the window. Stretching is
    // what the second pass does, on purpose: the scene was drawn at a size of
    // its own.
    const Matrix44f projection =
        matrix::perspective(60.0_deg, 1.0f, 0.1f, 20.0f);

    GPU_TRY(m_scene_program.set("model", model));
    GPU_TRY(m_scene_program.set("view", view));
    GPU_TRY(m_scene_program.set("projection", projection));

    {
        gpu::PassDesc desc;
        desc.width = TARGET;
        desc.height = TARGET;
        desc.target = m_target.handle();
        desc.color = Vector4f(0.07f, 0.07f, 0.1f, 1.0f);
        GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
        GPU_TRY(gpu::drawIndexed(m_scene, m_vertices, m_indices));
    }

    GPU_TRY(m_color.bind(UNIT));

    const std::uint32_t half = p_frame.width / 2u;
    {
        gpu::PassDesc desc;
        desc.width = half;
        desc.height = p_frame.height;
        desc.color = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
        GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
        GPU_TRY(gpu::drawWithoutVertices(m_blit, 3u));
    }
    {
        gpu::PassDesc desc;
        desc.x = half;
        desc.width = p_frame.width - half;
        desc.height = p_frame.height;
        desc.color = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
        GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
        GPU_TRY(gpu::drawWithoutVertices(m_process, 3u));
    }

    return gpu::success();
}

} // namespace examples
