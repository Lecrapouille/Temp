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

#include "00_Basics/08_Mandelbrot.hpp"

#include <cmath>

namespace examples
{

namespace
{

constexpr const char* VERTEX = R"(#version 450 core
out vec2 vClip;

void main()
{
    vec2 corners[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    vClip = corners[gl_VertexID];
    gl_Position = vec4(vClip, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT = R"(#version 450 core
in vec2 vClip;

uniform vec2 center;
uniform float scale;
uniform float aspect;

out vec4 oColor;

void main()
{
    vec2 c = center + vec2(vClip.x * aspect, vClip.y) * scale;
    vec2 z = vec2(0.0);
    int n = 0;
    const int limit = 80;

    for (; n < limit; ++n)
    {
        if (dot(z, z) > 4.0)
        {
            break;
        }
        z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
    }

    if (n == limit)
    {
        oColor = vec4(0.02, 0.02, 0.05, 1.0);
        return;
    }

    float t = float(n) / float(limit);
    oColor = vec4(0.5 + 0.5 * cos(3.0 + t * 6.2832 + vec3(0.0, 0.6, 1.0)), 1.0);
}
)";

} // namespace

//------------------------------------------------------------------------------
std::string Mandelbrot::description() const
{
    return "No vertex buffer. The shader builds a triangle covering the window "
           "from gl_VertexID, and every pixel is an iteration of z = z² + c. "
           "The mouse is the point the zoom keeps still; time closes in on it. "
           "The counters below should show one program, one pipeline, and "
           "nothing else.";
}

//------------------------------------------------------------------------------
gpu::Status Mandelbrot::setUp()
{
    GPU_TRY_ASSIGN(program, gpu::Program::fromSources(VERTEX, FRAGMENT));
    m_program = std::move(program);

    GPU_TRY_ASSIGN(pipeline,
                   gpu::Pipeline::create(m_program, gpu::VertexLayout{}));
    m_pipeline = std::move(pipeline);
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status Mandelbrot::draw(Frame const& p_frame)
{
    // The complex number under the mouse is computed at the scale of the last
    // frame, then the view is placed so that the same number is still under the
    // mouse at the new scale. Moving the cursor without zooming does nothing;
    // time is what closes in, and the cursor is what chooses where.
    const float scale = 1.5f * std::exp(-0.18f * p_frame.total);
    const Vector2f mouse = p_frame.mouseInClipSpace();
    const Vector2f under(
        m_center.x + (mouse.x * p_frame.aspect() * m_scale),
        m_center.y + (mouse.y * m_scale));
    m_center.x = under.x - (mouse.x * p_frame.aspect() * scale);
    m_center.y = under.y - (mouse.y * scale);
    m_scale = scale;

    GPU_TRY(m_program.set("center", m_center));
    GPU_TRY(m_program.set("scale", m_scale));
    GPU_TRY(m_program.set("aspect", p_frame.aspect()));

    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));

    return gpu::drawWithoutVertices(m_pipeline, 3u);
}

} // namespace examples
