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

#include "Render/DebugDraw.hpp"

#include "GPU/Core/Layout.hpp"
#include "GPU/Draw.hpp"
#include "GPU/Shader.hpp"

#include <array>

namespace render
{

namespace
{

constexpr char const* DEBUG_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 color;

uniform mat4 view;
uniform mat4 projection;

out vec3 vColor;

void main()
{
    vColor = color;
    gl_Position = projection * view * vec4(position, 1.0);
}
)";

constexpr char const* DEBUG_FRAGMENT = R"(#version 450 core
in vec3 vColor;
out vec4 oColor;

void main()
{
    oColor = vec4(vColor, 1.0);
}
)";

} // namespace

//------------------------------------------------------------------------------
gloop::Status DebugDraw::ensureInitialized()
{
    if (m_ready)
    {
        return gloop::success();
    }

    GPU_TRY_ASSIGN(program,
                   gpu::Program::fromSources(DEBUG_VERTEX, DEBUG_FRAGMENT));
    m_program = std::move(program);

    const gpu::VertexLayout layout =
        GPU_LAYOUT(DebugVertex, position, color);
    gpu::RenderState state;
    state.depth_test = true;
    state.depth_write = true;
    state.cull = gpu::CullMode::None;
    state.primitive = gpu::Primitive::Lines;
    GPU_TRY_ASSIGN(pipeline,
                   gpu::Pipeline::create<DebugVertex>(m_program, layout, state));
    m_pipeline = std::move(pipeline);

    GPU_TRY_ASSIGN(vertices,
                   gpu::Buffer<DebugVertex>::create(1024u,
                                                    gpu::BufferKind::Vertex,
                                                    gpu::BufferUsage::Dynamic));
    m_vertices = std::move(vertices);
    m_ready = true;
    return gloop::success();
}

//------------------------------------------------------------------------------
void DebugDraw::clear()
{
    m_pending.clear();
}

//------------------------------------------------------------------------------
void DebugDraw::line(Vector3f const& p_a,
                     Vector3f const& p_b,
                     Vector3f const& p_color)
{
    m_pending.push_back(DebugVertex{ p_a, p_color });
    m_pending.push_back(DebugVertex{ p_b, p_color });
}

//------------------------------------------------------------------------------
void DebugDraw::box(AABB const& p_local_bounds,
                    Matrix44f const& p_world_matrix,
                    Vector3f const& p_color)
{
    if (p_local_bounds.empty())
    {
        return;
    }

    const AABB world = p_local_bounds.transformed(p_world_matrix);
    const Vector3f& mn = world.min;
    const Vector3f& mx = world.max;
    const Vector3f corners[8]{
        Vector3f(mn.x, mn.y, mn.z), Vector3f(mx.x, mn.y, mn.z),
        Vector3f(mx.x, mx.y, mn.z), Vector3f(mn.x, mx.y, mn.z),
        Vector3f(mn.x, mn.y, mx.z), Vector3f(mx.x, mn.y, mx.z),
        Vector3f(mx.x, mx.y, mx.z), Vector3f(mn.x, mx.y, mx.z)
    };
    const std::array<std::pair<int, int>, 12u> edges{ {
        { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
        { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
        { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
    } };
    for (auto const& edge : edges)
    {
        line(corners[edge.first], corners[edge.second], p_color);
    }
}

//------------------------------------------------------------------------------
void DebugDraw::ray(Vector3f const& p_origin,
                    Vector3f const& p_direction,
                    float p_length,
                    Vector3f const& p_color)
{
    line(p_origin, p_origin + (p_direction * p_length), p_color);
}

//------------------------------------------------------------------------------
gloop::Status DebugDraw::flush(CameraFrame const& p_camera)
{
    if (m_pending.empty())
    {
        return gloop::success();
    }
    GPU_TRY(ensureInitialized());

    if (m_pending.size() > m_vertices.count())
    {
        GPU_TRY_ASSIGN(grown,
                       gpu::Buffer<DebugVertex>::create(
                           m_pending.size(),
                           gpu::BufferKind::Vertex,
                           gpu::BufferUsage::Dynamic));
        m_vertices = std::move(grown);
    }

    GPU_TRY(m_vertices.write(std::span<DebugVertex const>(m_pending)));
    GPU_TRY(m_program.set("view", p_camera.view));
    GPU_TRY(m_program.set("projection", p_camera.projection));
    return gpu::draw(m_pipeline, m_vertices.handle(), m_pending.size());
}

} // namespace render
