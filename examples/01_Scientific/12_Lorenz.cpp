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

#include "01_Scientific/12_Lorenz.hpp"

#include "Math/Transformation.hpp"

#include <algorithm>
#include <cmath>

using namespace units::literals;

namespace examples
{

namespace
{

constexpr std::size_t CAPACITY = 20000u;
constexpr std::size_t STEPS_PER_FRAME = 40u;
constexpr float DT = 0.005f;
constexpr float SIGMA = 10.0f;
constexpr float RHO = 28.0f;
constexpr float BETA = 8.0f / 3.0f;

constexpr const char* VERTEX = R"(#version 450 core
in vec3 position;
in vec3 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vColor;

void main()
{
    vColor = color;
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)";

constexpr const char* FRAGMENT = R"(#version 450 core
in vec3 vColor;
out vec4 oColor;

void main()
{
    oColor = vec4(vColor, 1.0);
}
)";

} // namespace

//------------------------------------------------------------------------------
std::string Lorenz::description() const
{
    return "A line that grows. Each step of the attractor is one push_back, so "
           "only the new point travels to the device. HeightMap rewrote every "
           "vertex; this appends. Twenty thousand points are reserved so the "
           "array never moves the ones already sent.";
}

//------------------------------------------------------------------------------
void Lorenz::step()
{
    const float x = m_state.x;
    const float y = m_state.y;
    const float z = m_state.z;
    m_state.x += DT * SIGMA * (y - x);
    m_state.y += DT * ((x * (RHO - z)) - y);
    m_state.z += DT * ((x * y) - (BETA * z));

    // The attractor lives around tens of units; bring it into the view of a
    // camera sitting a few units away.
    const Vector3f point(m_state.x * 0.04f,
                         (m_state.z * 0.04f) - 1.0f,
                         m_state.y * 0.04f);
    const Vector3f color(
        0.5f + (0.5f * std::tanh(m_state.x * 0.05f)),
        0.35f + (0.45f * std::tanh(m_state.y * 0.05f)),
        0.7f + (0.3f * std::tanh((m_state.z - 25.0f) * 0.04f)));
    m_trail.push_back(Vertex{ point, color });
}

//------------------------------------------------------------------------------
gpu::Status Lorenz::setUp()
{
    GPU_TRY_ASSIGN(program, gpu::Program::fromSources(VERTEX, FRAGMENT));
    m_program = std::move(program);

    m_trail.reserve(CAPACITY);
    m_state = Vector3f(0.1f, 0.0f, 0.0f);
    // Enough of a trail that the first frame already reads as the attractor,
    // including the ten frames --check draws.
    for (std::size_t i = 0u; i < 800u; ++i)
    {
        step();
    }

    gpu::RenderState state;
    state.primitive = gpu::Primitive::LineStrip;
    state.depth_test = true;

    const gpu::VertexLayout layout = GPU_LAYOUT(Vertex, position, color);
    GPU_TRY_ASSIGN(pipeline,
                   gpu::Pipeline::create<Vertex>(m_program, layout, state));
    m_pipeline = std::move(pipeline);
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status Lorenz::draw(Frame const& p_frame)
{
    const std::size_t room = (m_trail.size() < CAPACITY)
                                 ? (CAPACITY - m_trail.size())
                                 : 0u;
    const std::size_t steps = std::min(STEPS_PER_FRAME, room);
    for (std::size_t i = 0u; i < steps; ++i)
    {
        step();
    }

    const Matrix44f identity(matrix::Identity);
    const Matrix44f model = matrix::rotate(
        identity,
        units::angle::radian_t(p_frame.total * 0.35f),
        Vector3f(0.15f, 1.0f, 0.1f));
    const Matrix44f view = matrix::lookAt(Vector3f(0.0f, 0.2f, 3.4f),
                                          Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 1.0f, 0.0f));
    const Matrix44f projection =
        matrix::perspective(50.0_deg, p_frame.aspect(), 0.1f, 20.0f);

    GPU_TRY(m_program.set("model", model));
    GPU_TRY(m_program.set("view", view));
    GPU_TRY(m_program.set("projection", projection));

    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = Vector4f(0.04f, 0.04f, 0.06f, 1.0f);
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));

    return gpu::draw(m_pipeline, m_trail);
}

} // namespace examples
