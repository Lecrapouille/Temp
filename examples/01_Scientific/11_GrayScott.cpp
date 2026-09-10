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

#include "01_Scientific/11_GrayScott.hpp"

#include <vector>

namespace examples
{

namespace
{

constexpr std::uint32_t SIZE = 256u;
constexpr std::uint32_t UNIT = 0u;

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

constexpr const char* STEP_FRAGMENT = R"(#version 450 core
in vec2 vUV;
uniform sampler2D previous;
uniform vec2 texel;
out vec4 oColor;

void main()
{
    vec2 centre = texture(previous, vUV).rg;
    vec2 laplacian = -centre;
    laplacian += texture(previous, vUV + vec2(texel.x, 0.0)).rg;
    laplacian += texture(previous, vUV - vec2(texel.x, 0.0)).rg;
    laplacian += texture(previous, vUV + vec2(0.0, texel.y)).rg;
    laplacian += texture(previous, vUV - vec2(0.0, texel.y)).rg;

    const float F = 0.037;
    const float K = 0.06;
    const float Du = 0.16;
    const float Dv = 0.08;
    float u = centre.r;
    float v = centre.g;
    float uvv = u * v * v;
    u += (Du * laplacian.r) - uvv + (F * (1.0 - u));
    v += (Dv * laplacian.g) + uvv - ((F + K) * v);
    oColor = vec4(clamp(u, 0.0, 1.0), clamp(v, 0.0, 1.0), 0.0, 1.0);
}
)";

constexpr const char* SHOW_FRAGMENT = R"(#version 450 core
in vec2 vUV;
uniform sampler2D image;
out vec4 oColor;

void main()
{
    float v = texture(image, vUV).g;
    oColor = vec4(0.5 + 0.5 * cos(3.2 + v * 6.5 + vec3(0.1, 0.7, 1.1)), 1.0);
}
)";

} // namespace

//------------------------------------------------------------------------------
std::string GrayScott::description() const
{
    return "The same ping-pong as Game of Life, holding two concentrations as "
           "32-bit floats rather than a bit per cell. U is fed, V is killed, "
           "and where they meet they make more V. The display pass is what "
           "turns those numbers into a picture; the simulation never heard of "
           "a colour.";
}

//------------------------------------------------------------------------------
gpu::Status GrayScott::makeField(gpu::Texture& p_texture,
                                 gpu::Framebuffer& p_target)
{
    gpu::TextureDesc desc;
    desc.kind = gpu::TextureKind::Texture2D;
    desc.format = gpu::PixelFormat::RGBA32F;
    desc.width = SIZE;
    desc.height = SIZE;
    desc.levels = 1u;
    desc.magnify = gpu::Filter::Nearest;
    desc.minify = gpu::Filter::Nearest;
    desc.wrap_x = gpu::Wrap::Repeat;
    desc.wrap_y = gpu::Wrap::Repeat;
    GPU_TRY_ASSIGN(texture, gpu::Texture::create(desc));
    p_texture = std::move(texture);

    GPU_TRY_ASSIGN(target, gpu::Framebuffer::create(p_texture));
    p_target = std::move(target);
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status GrayScott::seed()
{
    std::vector<float> pixels(static_cast<std::size_t>(SIZE) * SIZE * 4u, 0.0f);
    for (std::uint32_t y = 0u; y < SIZE; ++y)
    {
        for (std::uint32_t x = 0u; x < SIZE; ++x)
        {
            const std::size_t at =
                ((static_cast<std::size_t>(y) * SIZE) + x) * 4u;
            pixels[at] = 1.0f;
            pixels[at + 3u] = 1.0f;
        }
    }

    const std::uint32_t mid = SIZE / 2u;
    const std::uint32_t blob = 12u;
    for (std::uint32_t y = mid - blob; y < mid + blob; ++y)
    {
        for (std::uint32_t x = mid - blob; x < mid + blob; ++x)
        {
            const std::size_t at =
                ((static_cast<std::size_t>(y) * SIZE) + x) * 4u;
            pixels[at] = 0.5f;
            pixels[at + 1u] = 0.25f;
        }
    }

    return m_field[0].write(std::as_bytes(std::span<const float>(pixels)));
}

//------------------------------------------------------------------------------
gpu::Status GrayScott::setUp()
{
    GPU_TRY_ASSIGN(step, gpu::Program::fromSources(SCREEN_VERTEX, STEP_FRAGMENT));
    m_step_program = std::move(step);
    GPU_TRY(m_step_program.set("previous", static_cast<int>(UNIT)));
    GPU_TRY(m_step_program.set(
        "texel",
        Vector2f(1.0f / static_cast<float>(SIZE), 1.0f / static_cast<float>(SIZE))));
    GPU_TRY_ASSIGN(step_pipe,
                   gpu::Pipeline::create(m_step_program, gpu::VertexLayout{}));
    m_step = std::move(step_pipe);

    GPU_TRY_ASSIGN(show, gpu::Program::fromSources(SCREEN_VERTEX, SHOW_FRAGMENT));
    m_show_program = std::move(show);
    GPU_TRY(m_show_program.set("image", static_cast<int>(UNIT)));
    GPU_TRY_ASSIGN(show_pipe,
                   gpu::Pipeline::create(m_show_program, gpu::VertexLayout{}));
    m_show = std::move(show_pipe);

    GPU_TRY(makeField(m_field[0], m_target[0]));
    GPU_TRY(makeField(m_field[1], m_target[1]));
    GPU_TRY(seed());
    m_current = 0;
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status GrayScott::draw(Frame const& p_frame)
{
    // Several steps per frame so a pattern appears in the few frames --check
    // draws, rather than after a minute of watching.
    for (int i = 0; i < 8; ++i)
    {
        const int next = 1 - m_current;
        GPU_TRY(m_field[m_current].bind(UNIT));
        gpu::PassDesc desc;
        desc.width = SIZE;
        desc.height = SIZE;
        desc.target = m_target[next].handle();
        desc.clear_color = false;
        desc.clear_depth = false;
        GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
        GPU_TRY(gpu::drawWithoutVertices(m_step, 3u));
        m_current = next;
    }

    GPU_TRY(m_field[m_current].bind(UNIT));
    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
    return gpu::drawWithoutVertices(m_show, 3u);
}

} // namespace examples
