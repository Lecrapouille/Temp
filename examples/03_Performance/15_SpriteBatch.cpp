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

#include "03_Performance/15_SpriteBatch.hpp"

#include <cmath>
#include <random>
#include <vector>

namespace examples
{

namespace
{

constexpr std::uint32_t SIDE = 320u;
constexpr std::uint32_t COUNT = SIDE * SIDE;
constexpr std::uint32_t UNIT = 0u;
constexpr std::uint32_t ATLAS = 128u;
constexpr std::uint32_t TILES = 4u;
constexpr std::uint32_t TILE_PX = ATLAS / TILES;

constexpr const char* VERTEX = R"(#version 450 core
in vec2 center;
in vec2 extent;
in float tile;
in float phase;

uniform float time;
uniform float aspect;

out vec2 vUV;

void main()
{
    vec2 corners[4] = vec2[4](vec2(-1.0, -1.0), vec2(1.0, -1.0),
                              vec2(-1.0, 1.0), vec2(1.0, 1.0));
    vec2 corner = corners[gl_VertexID];
    vec2 uv = (corner + 1.0) * 0.5;

    float col = mod(tile, 4.0);
    float row = floor(tile / 4.0);
    vUV = (vec2(col, row) + uv) / 4.0;

    float wobble = 0.012 * sin(time * 1.6 + phase);
    vec2 place = center + (corner * extent) + vec2(wobble, 0.0);
    gl_Position = vec4(place.x / aspect, place.y, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT = R"(#version 450 core
in vec2 vUV;
uniform sampler2D atlas;
out vec4 oColor;

void main()
{
    vec4 colour = texture(atlas, vUV);
    // A slight inset so neighbouring sprites do not fuse into a sheet.
    vec2 edge = abs(vUV * 4.0 - floor(vUV * 4.0) - 0.5);
    if (max(edge.x, edge.y) > 0.46)
    {
        discard;
    }
    oColor = colour;
}
)";

} // namespace

//------------------------------------------------------------------------------
std::string SpriteBatch::description() const
{
    return "A hundred thousand sprites, one draw. The four corners are built "
           "from gl_VertexID; the buffer holds one record per sprite, and every "
           "field is marked perInstance() so the hardware reads it once per "
           "object. The atlas is sixteen tiles computed here. The overlay "
           "should show one draw call, whatever the count.";
}

//------------------------------------------------------------------------------
gpu::Status SpriteBatch::makeAtlas()
{
    gpu::TextureDesc desc;
    desc.kind = gpu::TextureKind::Texture2D;
    desc.format = gpu::PixelFormat::RGBA8;
    desc.width = ATLAS;
    desc.height = ATLAS;
    desc.magnify = gpu::Filter::Nearest;
    desc.minify = gpu::Filter::Nearest;
    desc.wrap_x = gpu::Wrap::ClampToEdge;
    desc.wrap_y = gpu::Wrap::ClampToEdge;

    GPU_TRY_ASSIGN(texture, gpu::Texture::create(desc));
    m_atlas = std::move(texture);

    std::vector<std::uint8_t> pixels(
        static_cast<std::size_t>(ATLAS) * ATLAS * 4u, 0u);
    for (std::uint32_t y = 0u; y < ATLAS; ++y)
    {
        for (std::uint32_t x = 0u; x < ATLAS; ++x)
        {
            const std::uint32_t col = x / TILE_PX;
            const std::uint32_t row = y / TILE_PX;
            const std::uint32_t tile = (row * TILES) + col;
            const float t = static_cast<float>(tile) / 15.0f;
            const std::size_t at =
                ((static_cast<std::size_t>(y) * ATLAS) + x) * 4u;
            pixels[at] = static_cast<std::uint8_t>(40.0f + (200.0f * t));
            pixels[at + 1u] =
                static_cast<std::uint8_t>(180.0f - (120.0f * t));
            pixels[at + 2u] =
                static_cast<std::uint8_t>(80.0f + (140.0f * (1.0f - t)));
            pixels[at + 3u] = 255u;
        }
    }

    return m_atlas.write(std::as_bytes(std::span<const std::uint8_t>(pixels)));
}

//------------------------------------------------------------------------------
gpu::Status SpriteBatch::setUp()
{
    GPU_TRY_ASSIGN(program, gpu::Program::fromSources(VERTEX, FRAGMENT));
    m_program = std::move(program);
    GPU_TRY(makeAtlas());
    GPU_TRY(m_program.set("atlas", static_cast<int>(UNIT)));

    std::vector<Sprite> seed(COUNT);
    std::mt19937 rng(7u);
    std::uniform_real_distribution<float> jitter(-0.35f, 0.35f);
    std::uniform_int_distribution<int> tile(0, 15);

    const float step = 2.0f / static_cast<float>(SIDE);
    const float half = step * 0.42f;
    for (std::uint32_t y = 0u; y < SIDE; ++y)
    {
        for (std::uint32_t x = 0u; x < SIDE; ++x)
        {
            Sprite& s = seed[(static_cast<std::size_t>(y) * SIDE) + x];
            s.center = Vector2f(-1.0f + ((static_cast<float>(x) + 0.5f) * step),
                                -1.0f + ((static_cast<float>(y) + 0.5f) * step));
            s.extent = Vector2f(half, half);
            s.tile = static_cast<float>(tile(rng));
            s.phase = jitter(rng) * 8.0f;
        }
    }

    GPU_TRY_ASSIGN(sprites,
                   gpu::Buffer<Sprite>::from(std::span<const Sprite>(seed),
                                             gpu::BufferKind::Vertex,
                                             gpu::BufferUsage::Immutable));
    m_sprites = std::move(sprites);
    m_count = COUNT;

    gpu::RenderState state;
    state.primitive = gpu::Primitive::TriangleStrip;

    const gpu::VertexLayout layout = gpu::describe<Sprite>(
        gpu::field(&Sprite::center, "center").perInstance(),
        gpu::field(&Sprite::extent, "extent").perInstance(),
        gpu::field(&Sprite::tile, "tile").perInstance(),
        gpu::field(&Sprite::phase, "phase").perInstance());

    GPU_TRY_ASSIGN(pipeline,
                   gpu::Pipeline::create<Sprite>(m_program, layout, state));
    m_pipeline = std::move(pipeline);
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status SpriteBatch::draw(Frame const& p_frame)
{
    GPU_TRY(m_program.set("time", p_frame.total));
    GPU_TRY(m_program.set("aspect", p_frame.aspect()));
    GPU_TRY(m_atlas.bind(UNIT));

    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = Vector4f(0.04f, 0.04f, 0.06f, 1.0f);
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));

    return gpu::drawInstanced(m_pipeline, m_sprites, 4u, m_count);
}

} // namespace examples
