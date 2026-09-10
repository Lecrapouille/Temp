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

#pragma once

#include "Common/Example.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A curve that grows a point per step, and only the new point travels.
//!
//! HeightMap rewrote every vertex. This appends. VertexArray::push_back()
//! marks the new end, so update() sends one point rather than the whole trail.
//! The trail is drawn as a line strip: the primitive belongs to the pipeline,
//! the way the triangle strip of 04 did.
//!
//! Twenty thousand points are reserved up front so that growing the array never
//! moves the ones already sent, and so the device memory is asked for once.
// ****************************************************************************
class Lorenz: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "12_Lorenz";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    struct Vertex
    {
        Vector3f position;
        Vector3f color;
    };

    void step();

    gpu::Program m_program;
    gpu::Pipeline m_pipeline;
    gpu::VertexArray<Vertex> m_trail;
    Vector3f m_state{ 0.1f, 0.0f, 0.0f };
};

} // namespace examples
