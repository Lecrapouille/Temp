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

#include "01_Scientific/10_GameOfLife.hpp"

#include <cstdint>
#include <random>
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
    int neighbours = 0;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            if ((x == 0) && (y == 0))
            {
                continue;
            }
            neighbours += int(texture(previous, vUV + (vec2(x, y) * texel)).r > 0.5);
        }
    }

    bool alive = texture(previous, vUV).r > 0.5;
    bool next = alive ? ((neighbours == 2) || (neighbours == 3))
                      : (neighbours == 3);
    oColor = vec4(next ? 1.0 : 0.0);
}
)";

constexpr const char* SHOW_FRAGMENT = R"(#version 450 core
in vec2 vUV;
uniform sampler2D image;
out vec4 oColor;

void main()
{
    float cell = texture(image, vUV).r;
    oColor = vec4(vec3(0.06, 0.08, 0.1) + (vec3(0.35, 0.85, 0.45) * cell), 1.0);
}
)";

} // namespace

//------------------------------------------------------------------------------
std::string GameOfLife::description() const
{
    return "Each cell is a pixel. A fullscreen pass reads this generation and "
           "writes the next into a second texture; the two swap. A shader cannot "
           "read the picture it is writing, which is why there are two of "
           "everything. The world wraps. This is how glumpy computed on the GPU.";
}

//------------------------------------------------------------------------------
gpu::Status GameOfLife::makeField(gpu::Texture& p_texture,
                                  gpu::Framebuffer& p_target)
{
    gpu::TextureDesc desc;
    desc.kind = gpu::TextureKind::Texture2D;
    desc.format = gpu::PixelFormat::R8;
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
gpu::Status GameOfLife::seed()
{
    std::vector<std::uint8_t> cells(static_cast<std::size_t>(SIZE) * SIZE, 0u);
    std::mt19937 rng(2026u);
    std::uniform_int_distribution<int> coin(0, 4);
    for (std::uint8_t& cell : cells)
    {
        cell = (coin(rng) == 0) ? 255u : 0u;
    }
    return m_field[0].write(std::as_bytes(std::span<const std::uint8_t>(cells)));
}

//------------------------------------------------------------------------------
gpu::Status GameOfLife::setUp()
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
gpu::Status GameOfLife::draw(Frame const& p_frame)
{
    const int next = 1 - m_current;

    GPU_TRY(m_field[m_current].bind(UNIT));
    {
        gpu::PassDesc desc;
        desc.width = SIZE;
        desc.height = SIZE;
        desc.target = m_target[next].handle();
        desc.clear_color = false;
        desc.clear_depth = false;
        GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
        GPU_TRY(gpu::drawWithoutVertices(m_step, 3u));
    }

    m_current = next;

    GPU_TRY(m_field[m_current].bind(UNIT));
    {
        gpu::PassDesc desc;
        desc.width = p_frame.width;
        desc.height = p_frame.height;
        desc.color = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
        GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
        GPU_TRY(gpu::drawWithoutVertices(m_show, 3u));
    }

    return gpu::success();
}

} // namespace examples
