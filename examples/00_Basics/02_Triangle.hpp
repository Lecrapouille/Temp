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
//! \brief One triangle, and the four objects it takes to draw anything.
//!
//! A shader that says how, a struct that says what one vertex is, a buffer holding
//! the vertices, and a pipeline that checks the three agree with each other. That
//! last one is the difference from writing this by hand: a field the shader does
//! not declare, a type that does not match, or a stride that is not the size of the
//! struct is a sentence on the screen at set up time instead of an empty window.
//!
//! The buffer is immutable, which is the honest description of a triangle whose
//! corners never move, and lets the driver put it wherever it reads fastest.
// ****************************************************************************
class Triangle: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "02_Triangle";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    // ------------------------------------------------------------------------
    //! \brief What one corner of the triangle is.
    //!
    //! Both fields in one struct, and therefore in one buffer, interleaved: the
    //! device reads a whole vertex from one place instead of gathering it from
    //! two. That is also why a vertex is a struct here rather than a set of
    //! parallel arrays.
    // ------------------------------------------------------------------------
    struct Vertex
    {
        Vector2f position;
        Vector3f color;
    };

    gpu::Program m_program;
    gpu::Pipeline m_pipeline;
    gpu::Buffer<Vertex> m_vertices;
    //! \brief Kept so that the description can show what the pipeline decided,
    //! which is the part a reader wants to see once and then never doubts again.
    std::string m_attributes;
};

} // namespace examples
