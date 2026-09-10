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

#include "03_Performance/16_IndirectDraw.hpp"

#include <cmath>
#include <random>
#include <vector>

namespace examples
{

namespace
{

constexpr std::uint32_t SIDE = 128u;
constexpr std::uint32_t COUNT = SIDE * SIDE;

constexpr const char* CULL = R"(#version 450 core
layout(local_size_x = 64) in;

struct Dot
{
    vec2 position;
};

layout(std430, binding = 0) readonly buffer All
{
    Dot incoming[];
};

layout(std430, binding = 1) writeonly buffer Kept
{
    Dot outgoing[];
};

layout(std430, binding = 2) buffer Command
{
    uint vertex_count;
    uint instance_count;
    uint first_vertex;
    uint first_instance;
};

uniform vec2 center;
uniform float radius;
uniform uint count;

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= count)
    {
        return;
    }

    vec2 p = incoming[i].position;
    vec2 d = p - center;
    if (dot(d, d) > radius * radius)
    {
        return;
    }

    uint slot = atomicAdd(vertex_count, 1u);
    outgoing[slot].position = p;
}
)";

constexpr const char* VERTEX = R"(#version 450 core
in vec2 position;

void main()
{
    gl_PointSize = 4.0;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT = R"(#version 450 core
out vec4 oColor;

void main()
{
    vec2 fromCentre = gl_PointCoord - vec2(0.5);
    if (dot(fromCentre, fromCentre) > 0.25)
    {
        discard;
    }
    oColor = vec4(0.95, 0.85, 0.35, 1.0);
}
)";

} // namespace

//------------------------------------------------------------------------------
std::string IndirectDraw::description() const
{
    return "Sixteen thousand points, and a circle that keeps some of them. A "
           "compute pass writes the survivors into a second buffer and the "
           "four words a drawIndirect command is. The CPU resets the count to "
           "zero; it never reads how many survived. The command barrier is "
           "what makes those words visible.";
}

//------------------------------------------------------------------------------
gpu::Status IndirectDraw::setUp()
{
    GPU_TRY_ASSIGN(cull, gpu::ComputeProgram::fromSource(CULL));
    m_cull = std::move(cull);
    GPU_TRY(m_cull.set("count", COUNT));

    std::vector<Dot> seed(COUNT);
    std::mt19937 rng(21u);
    std::uniform_real_distribution<float> jitter(-0.35f, 0.35f);
    const float step = 2.0f / static_cast<float>(SIDE);
    for (std::uint32_t y = 0u; y < SIDE; ++y)
    {
        for (std::uint32_t x = 0u; x < SIDE; ++x)
        {
            Dot& d = seed[(static_cast<std::size_t>(y) * SIDE) + x];
            d.position = Vector2f(
                -1.0f + ((static_cast<float>(x) + 0.5f + jitter(rng)) * step),
                -1.0f + ((static_cast<float>(y) + 0.5f + jitter(rng)) * step));
        }
    }

    GPU_TRY_ASSIGN(all,
                   gpu::Buffer<Dot>::from(std::span<const Dot>(seed),
                                          gpu::BufferKind::Storage,
                                          gpu::BufferUsage::Storage));
    m_all = std::move(all);
    GPU_TRY_ASSIGN(kept,
                   gpu::Buffer<Dot>::create(COUNT,
                                            gpu::BufferKind::Storage,
                                            gpu::BufferUsage::Storage));
    m_kept = std::move(kept);
    GPU_TRY_ASSIGN(command,
                   gpu::Buffer<gpu::DrawIndirectCommand>::create(
                       1u,
                       gpu::BufferKind::Storage,
                       gpu::BufferUsage::Storage));
    m_command = std::move(command);
    m_count = COUNT;

    GPU_TRY(m_cull.bind("All", m_all));
    GPU_TRY(m_cull.bind("Kept", m_kept));
    GPU_TRY(m_cull.bind("Command", m_command));

    GPU_TRY_ASSIGN(program, gpu::Program::fromSources(VERTEX, FRAGMENT));
    m_draw_program = std::move(program);

    gpu::RenderState state;
    state.primitive = gpu::Primitive::Points;

    const gpu::VertexLayout layout = GPU_LAYOUT(Dot, position);
    GPU_TRY_ASSIGN(pipeline,
                   gpu::Pipeline::create<Dot>(m_draw_program, layout, state));
    m_draw = std::move(pipeline);
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status IndirectDraw::draw(Frame const& p_frame)
{
    // A circle that wanders, so --check still has something to draw without a
    // mouse, and so the cull is visible as a moving hole in the field.
    const Vector2f mouse = p_frame.mouseInClipSpace();
    const bool steered =
        (std::abs(mouse.x) > 0.02f) || (std::abs(mouse.y) > 0.02f);
    const Vector2f center =
        steered ? mouse
                : Vector2f(0.55f * std::sin(p_frame.total * 0.7f),
                           0.40f * std::cos(p_frame.total * 0.5f));
    const float radius = 0.42f;

    const gpu::DrawIndirectCommand reset{ 0u, 1u, 0u, 0u };
    GPU_TRY(m_command.write(reset, 0u));

    GPU_TRY(m_cull.set("center", center));
    GPU_TRY(m_cull.set("radius", radius));
    GPU_TRY(m_cull.dispatchItems(m_count));
    gpu::barrier(gpu::Barrier::VertexAttrib | gpu::Barrier::Command);

    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = Vector4f(0.03f, 0.03f, 0.05f, 1.0f);
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
    return gpu::drawIndirect(m_draw, m_kept, m_command);
}

} // namespace examples
