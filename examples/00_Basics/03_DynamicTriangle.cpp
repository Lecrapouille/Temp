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

#include "00_Basics/03_DynamicTriangle.hpp"

namespace examples
{

namespace
{

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

//! \brief Which element of the array the mouse moves. The other two are written
//! once, at set up, and never sent again.
constexpr std::size_t APEX = 2u;

} // namespace

//------------------------------------------------------------------------------
std::string DynamicTriangle::description() const
{
    return "The apex follows the mouse. A gpu::VertexArray is a std::vector that "
           "remembers which of its elements were written, so one vertex out of "
           "three travels to the device each frame rather than all three. Nothing "
           "here asks for an upload: writing to the array is the whole of it.";
}

//------------------------------------------------------------------------------
gpu::Status DynamicTriangle::setUp()
{
    GPU_TRY_ASSIGN(program,
                   gpu::Program::fromSources(VERTEX_SHADER, FRAGMENT_SHADER));
    m_program = std::move(program);

    const gpu::VertexLayout layout = GPU_LAYOUT(Vertex, position, color);
    GPU_TRY_ASSIGN(pipeline, gpu::Pipeline::create<Vertex>(m_program, layout));
    m_pipeline = std::move(pipeline);

    // No device memory yet: the array reserves it on the first update, which is
    // also the first draw. Nothing has to be created in the right order by hand.
    m_corners.resize(3u);
    m_corners.set(0u, Vertex{ Vector2f(-0.8f, -0.7f), Vector3f(1.0f, 0.2f, 0.2f) });
    m_corners.set(1u, Vertex{ Vector2f(0.8f, -0.7f), Vector3f(0.2f, 1.0f, 0.2f) });
    m_corners.set(APEX, Vertex{ Vector2f(0.0f, 0.7f), Vector3f(0.3f, 0.4f, 1.0f) });

    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status DynamicTriangle::draw(Frame const& p_frame)
{
    // One element written, so one element is what the next draw sends. set() is
    // used rather than a reference into the array precisely because it is the
    // writing that has to be noticed.
    m_corners.set(APEX,
                  Vertex{ p_frame.mouseInClipSpace(),
                          Vector3f(0.3f, 0.4f, 1.0f) });

    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = Vector4f(0.1f, 0.1f, 0.15f, 1.0f);
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));

    // This overload of draw() sends what changed before drawing. The alternative,
    // asking for the upload here, is the line every demo forgets once and spends an
    // afternoon on.
    return gpu::draw(m_pipeline, m_corners);
}

} // namespace examples
