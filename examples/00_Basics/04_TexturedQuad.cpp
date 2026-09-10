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

#include "00_Basics/04_TexturedQuad.hpp"

#include <array>
#include <cmath>
#include <vector>

namespace examples
{

namespace
{

//! \brief The texture unit the image is bound to and the shader is told about.
//! Written once, used twice, which is the point: the two must agree, and here they
//! cannot drift apart.
constexpr std::uint32_t UNIT = 0u;

//! \brief How many pixels across the checkerboard is, and how large one square of
//! it is.
constexpr std::uint32_t SIZE = 256u;
constexpr std::uint32_t SQUARE = 32u;

constexpr const char* VERTEX_SHADER = R"(#version 450 core

in vec2 position;
in vec2 uv;

uniform float scale;

out vec2 vUV;

void main()
{
    vUV = uv * scale;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT_SHADER = R"(#version 450 core

in vec2 vUV;

uniform sampler2D image;

out vec4 oColor;

void main()
{
    oColor = texture(image, vUV);
}
)";

} // namespace

//------------------------------------------------------------------------------
std::string TexturedQuad::description() const
{
    return "A checkerboard computed in C++, sent once, and read by the shader. The "
           "sampler is set by giving it the number of the unit the texture is bound "
           "to. The corners are drawn as a triangle strip, which is part of the "
           "render state the pipeline was built with rather than an argument to the "
           "draw. Nearest filtering and repeating wrap, so the squares stay square "
           "and the image tiles as it zooms.";
}

//------------------------------------------------------------------------------
gpu::Status TexturedQuad::makeTexture()
{
    gpu::TextureDesc desc;
    desc.kind = gpu::TextureKind::Texture2D;
    desc.format = gpu::PixelFormat::RGBA8;
    desc.width = SIZE;
    desc.height = SIZE;
    // Nearest, because a checkerboard is data with edges rather than a photograph:
    // mixing neighbours would invent grey where the design says black or white.
    desc.magnify = gpu::Filter::Nearest;
    desc.minify = gpu::Filter::Nearest;
    desc.wrap_x = gpu::Wrap::Repeat;
    desc.wrap_y = gpu::Wrap::Repeat;

    GPU_TRY_ASSIGN(texture, gpu::Texture::create(desc));
    m_texture = std::move(texture);

    std::vector<std::uint8_t> pixels(
        static_cast<std::size_t>(SIZE) * SIZE * 4u, 0u);
    for (std::uint32_t y = 0u; y < SIZE; ++y)
    {
        for (std::uint32_t x = 0u; x < SIZE; ++x)
        {
            const bool light = (((x / SQUARE) + (y / SQUARE)) % 2u) == 0u;
            // A gradient over the checkerboard, so that the tiling is visible: with
            // flat squares alone, one copy of the texture looks like the next.
            const auto ramp = static_cast<std::uint8_t>((x * 255u) / SIZE);

            const std::size_t at =
                ((static_cast<std::size_t>(y) * SIZE) + x) * 4u;
            pixels[at] = light ? 230u : 40u;
            pixels[at + 1u] = ramp;
            pixels[at + 2u] = light ? 90u : 160u;
            pixels[at + 3u] = 255u;
        }
    }

    return m_texture.write(std::as_bytes(std::span<const std::uint8_t>(pixels)));
}

//------------------------------------------------------------------------------
gpu::Status TexturedQuad::setUp()
{
    GPU_TRY_ASSIGN(program,
                   gpu::Program::fromSources(VERTEX_SHADER, FRAGMENT_SHADER));
    m_program = std::move(program);

    GPU_TRY(makeTexture());

    // The order a triangle strip reads: bottom left, bottom right, top left, top
    // right. Going round the outline instead is what turns a strip into a bow tie.
    const std::array<Vertex, 4u> corners{
        Vertex{ Vector2f(-0.9f, -0.9f), Vector2f(0.0f, 0.0f) },
        Vertex{ Vector2f(0.9f, -0.9f), Vector2f(1.0f, 0.0f) },
        Vertex{ Vector2f(-0.9f, 0.9f), Vector2f(0.0f, 1.0f) },
        Vertex{ Vector2f(0.9f, 0.9f), Vector2f(1.0f, 1.0f) }
    };
    GPU_TRY_ASSIGN(vertices,
                   gpu::Buffer<Vertex>::from(std::span<const Vertex>(corners),
                                             gpu::BufferKind::Vertex,
                                             gpu::BufferUsage::Immutable));
    m_vertices = std::move(vertices);

    gpu::RenderState state;
    state.primitive = gpu::Primitive::TriangleStrip;

    const gpu::VertexLayout layout = GPU_LAYOUT(Vertex, position, uv);
    GPU_TRY_ASSIGN(pipeline,
                   gpu::Pipeline::create<Vertex>(m_program, layout, state));
    m_pipeline = std::move(pipeline);

    // Which unit the sampler reads from. Set once here rather than every frame,
    // because it does not change: what a uniform holds survives until it is set
    // again.
    return m_program.set("image", static_cast<int>(UNIT));
}

//------------------------------------------------------------------------------
gpu::Status TexturedQuad::draw(Frame const& p_frame)
{
    // Between one and three copies of the texture across the quad, which is what
    // makes the repeating wrap visible.
    const float scale = 2.0f + (1.0f * std::sin(p_frame.total * 0.7f));
    GPU_TRY(m_program.set("scale", scale));

    GPU_TRY(m_texture.bind(UNIT));

    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = Vector4f(0.05f, 0.05f, 0.08f, 1.0f);
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));

    return gpu::draw(m_pipeline, m_vertices);
}

} // namespace examples
