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
//! \brief A solid in three dimensions: indices, depth, culling and matrices.
//!
//! Everything the previous four examples left out arrives at once, because these
//! four things are what turn a flat picture into a solid and they are useless one
//! at a time.
//!
//! Indices, so that a corner shared by several triangles is stored once and named
//! several times. Depth testing, so that a face nearer the eye covers one further
//! away whatever order they were drawn in. Back face culling, so that half the
//! triangles of a closed shape are dropped before they cost anything. And three
//! matrices, so that a shape described once can be placed, looked at, and
//! projected.
//!
//! The depth test and the culling live in the render state, which the pipeline was
//! built with: they are not switched on before the draw and forgotten afterwards.
//! Two examples wanting different ones are two pipelines, and neither can leave the
//! device in a state the other did not ask for.
// ****************************************************************************
class IndexedCube: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "05_IndexedCube";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    // ------------------------------------------------------------------------
    //! \brief One corner of the cube, with the direction the face it belongs to
    //! points in.
    //!
    //! Twenty four of them for eight geometric corners, because a corner of a cube
    //! belongs to three faces pointing three different ways, and a vertex holds one
    //! normal. Sharing the eight would give a smooth ball of a cube: the sharing
    //! that indices allow is sharing of identical vertices, not of positions.
    // ------------------------------------------------------------------------
    struct Vertex
    {
        Vector3f position;
        Vector3f normal;
        Vector3f color;
    };

    gpu::Program m_program;
    gpu::Pipeline m_pipeline;
    gpu::Buffer<Vertex> m_vertices;
    gpu::Buffer<std::uint16_t> m_indices;
};

} // namespace examples
