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

#include "02_Compute/13_ComputeParticles.hpp"

#include <cmath>
#include <random>
#include <vector>

namespace examples
{

namespace
{

constexpr std::uint32_t COUNT = 4096u;

constexpr const char* STEP = R"(#version 450 core
layout(local_size_x = 64) in;

struct Particle
{
    vec2 position;
    vec2 velocity;
    vec4 color;
};

layout(std430, binding = 0) buffer Particles
{
    Particle particles[];
};

uniform float dt;
uniform uint count;

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= count)
    {
        return;
    }

    Particle p = particles[i];
    p.position += p.velocity * dt;
    if (abs(p.position.x) > 0.95)
    {
        p.velocity.x = -p.velocity.x;
        p.position.x = clamp(p.position.x, -0.95, 0.95);
    }
    if (abs(p.position.y) > 0.95)
    {
        p.velocity.y = -p.velocity.y;
        p.position.y = clamp(p.position.y, -0.95, 0.95);
    }
    particles[i] = p;
}
)";

constexpr const char* VERTEX = R"(#version 450 core
in vec2 position;
in vec4 color;
out vec4 vColor;

void main()
{
    vColor = color;
    gl_PointSize = 3.0;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT = R"(#version 450 core
in vec4 vColor;
out vec4 oColor;

void main()
{
    vec2 fromCentre = gl_PointCoord - vec2(0.5);
    if (dot(fromCentre, fromCentre) > 0.25)
    {
        discard;
    }
    oColor = vColor;
}
)";

} // namespace

//------------------------------------------------------------------------------
std::string ComputeParticles::description() const
{
    return "Four thousand particles bounce in a box. A compute pass writes "
           "their next place into the same Buffer a draw then reads as "
           "vertices: there is no copy. The barrier between the two is what "
           "makes the writes visible. Game of Life needed two textures for "
           "this; here one buffer is enough, because a compute shader writes "
           "wherever it wants.";
}

//------------------------------------------------------------------------------
gpu::Status ComputeParticles::setUp()
{
    GPU_TRY_ASSIGN(step, gpu::ComputeProgram::fromSource(STEP));
    m_step = std::move(step);
    GPU_TRY(m_step.set("count", COUNT));

    std::vector<Particle> seed(COUNT);
    std::mt19937 rng(13u);
    std::uniform_real_distribution<float> pos(-0.8f, 0.8f);
    std::uniform_real_distribution<float> vel(-0.4f, 0.4f);
    std::uniform_real_distribution<float> hue(0.0f, 1.0f);
    for (Particle& p : seed)
    {
        p.position = Vector2f(pos(rng), pos(rng));
        p.velocity = Vector2f(vel(rng), vel(rng));
        const float t = hue(rng);
        p.color = Vector4f(0.4f + (0.6f * t),
                           0.5f + (0.4f * (1.0f - t)),
                           0.9f,
                           0.9f);
    }

    GPU_TRY_ASSIGN(particles,
                   gpu::Buffer<Particle>::from(std::span<const Particle>(seed),
                                               gpu::BufferKind::Storage,
                                               gpu::BufferUsage::Storage));
    m_particles = std::move(particles);
    GPU_TRY(m_step.bind("Particles", m_particles));

    GPU_TRY_ASSIGN(program, gpu::Program::fromSources(VERTEX, FRAGMENT));
    m_draw_program = std::move(program);

    gpu::RenderState state;
    state.primitive = gpu::Primitive::Points;
    state.blend = gpu::Blend::additive();

    const gpu::VertexLayout layout = GPU_LAYOUT(Particle, position, color);
    GPU_TRY_ASSIGN(pipeline,
                   gpu::Pipeline::create<Particle>(
                       m_draw_program, layout, state));
    m_draw = std::move(pipeline);
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status ComputeParticles::draw(Frame const& p_frame)
{
    GPU_TRY(m_step.set("dt", p_frame.elapsed));
    GPU_TRY(m_step.dispatchItems(COUNT));
    gpu::barrier(gpu::Barrier::VertexAttrib);

    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = Vector4f(0.02f, 0.02f, 0.04f, 1.0f);
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
    return gpu::draw(m_draw, m_particles);
}

} // namespace examples
