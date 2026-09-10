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

#include "00_Basics/02_Triangle.hpp"

#include <array>

namespace examples
{

namespace
{

//! \brief The names here, position and color, are what the pipeline looks for in
//! the layout. Nothing else ties the two together: no attribute number to keep in
//! step on both sides, which is the numbering mistake this design removes.
constexpr const char* VERTEX_SHADER = R"(#version 450 core

in vec2 position;
in vec3 color;

out vec3 vColor;

void main()
{
    vColor = color;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT_SHADER = R"(#version 450 core

in vec3 vColor;
out vec4 oColor;

void main()
{
    oColor = vec4(vColor, 1.0);
}
)";

} // namespace

//------------------------------------------------------------------------------
std::string Triangle::description() const
{
    std::string text =
        "A shader, a vertex struct, a buffer and a pipeline. The pipeline is what "
        "checks that the struct can feed the shader, at set up time, and says which "
        "field goes to which attribute:\n\n";
    return text + m_attributes;
}

//------------------------------------------------------------------------------
gpu::Status Triangle::setUp()
{
    GPU_TRY_ASSIGN(program,
                   gpu::Program::fromSources(VERTEX_SHADER, FRAGMENT_SHADER));
    m_program = std::move(program);

    const std::array<Vertex, 3u> corners{
        Vertex{ Vector2f(-0.8f, -0.6f), Vector3f(1.0f, 0.0f, 0.0f) },
        Vertex{ Vector2f(0.8f, -0.6f), Vector3f(0.0f, 1.0f, 0.0f) },
        Vertex{ Vector2f(0.0f, 0.8f), Vector3f(0.0f, 0.0f, 1.0f) }
    };

    // Immutable, because a triangle whose corners never move is exactly that, and
    // saying so lets the driver place the memory where the device reads it fastest.
    // Asking to write it later is refused, at compile time for the wrong type and
    // at run time for the wrong usage, rather than silently doing nothing.
    GPU_TRY_ASSIGN(vertices,
                   gpu::Buffer<Vertex>::from(std::span<const Vertex>(corners),
                                             gpu::BufferKind::Vertex,
                                             gpu::BufferUsage::Immutable));
    m_vertices = std::move(vertices);

    // The one place the shape of a vertex is written down. The macro takes the
    // struct and the fields by name, works out the offsets with offsetof and the
    // types with a trait, and computes the stride from sizeof: three numbers that
    // used to be typed by hand and had to agree with each other.
    const gpu::VertexLayout layout = GPU_LAYOUT(Vertex, position, color);

    // The templated create() also checks the layout against the struct it claims to
    // describe, so a field added to Vertex and forgotten in the layout is caught
    // here rather than read as garbage by the device.
    GPU_TRY_ASSIGN(pipeline,
                   gpu::Pipeline::create<Vertex>(m_program, layout));
    m_pipeline = std::move(pipeline);

    m_attributes = m_pipeline.describeAttributes();

    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status Triangle::draw(Frame const& p_frame)
{
    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = Vector4f(0.1f, 0.1f, 0.15f, 1.0f);
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));

    // How many vertices comes from the buffer, so there is no count to keep in step
    // with it. The pipeline knows the size of a vertex and refuses a buffer of
    // anything else.
    return gpu::draw(m_pipeline, m_vertices);
}

} // namespace examples
