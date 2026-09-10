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
//! \brief Vertices that move, sent as cheaply as they can be.
//!
//! A gpu::VertexArray is a std::vector whose changes are noticed. Write to it as
//! to any container; what was written, and only that, travels to the device on the
//! next draw. Here the apex of the triangle follows the mouse and the two feet
//! never move, so one vertex out of three is sent per frame.
//!
//! That is not an optimisation looking for a problem. It is the shape of every
//! demo that computes something on the CPU and shows it: a curve being integrated,
//! a mesh being deformed, a plot growing a point at a time. The previous layer of
//! this library got this wrong in a way worth remembering, and the note in
//! VertexArray.hpp says how.
// ****************************************************************************
class DynamicTriangle: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "03_DynamicTriangle";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    struct Vertex
    {
        Vector2f position;
        Vector3f color;
    };

    gpu::Program m_program;
    gpu::Pipeline m_pipeline;
    //! \brief The CPU side and the device side of the same three vertices, kept in
    //! step by the array itself.
    gpu::VertexArray<Vertex> m_corners;
};

} // namespace examples
