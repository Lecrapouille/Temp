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

// ****************************************************************************
//! \brief The three matrices every pass of a frame reads.
//!
//! Written once, read by three programs. That is the reason a uniform block
//! exists: Program::set() would have copied them three times, into three places
//! the driver keeps.
//!
//! GPU_STD140 is at file scope on purpose. It specialises a template in
//! gpu::std140, which a type hidden in a class or an anonymous namespace cannot
//! do, and the compiler would then treat the struct as never described.
// ****************************************************************************
struct Transforms
{
    Matrix44f projection;
    Matrix44f view;
    Matrix44f model;
};
GPU_STD140(Transforms, projection, view, model);

namespace examples
{

// ****************************************************************************
//! \brief One mesh, three ways of drawing it, one block of matrices.
//!
//! The previous layer of this library refused to let a vertex array serve a
//! second program. A shadow pass, a normals view and a wireframe were therefore
//! three copies of the same vertices. Here they are one Buffer<Vertex> and three
//! pipelines, and the overlay's vertex-reader count is what says the sharing is
//! real: the lit pass reads everything, the normals pass drops the colour the
//! linker never needed, the wireframe drops both, and each distinct way of
//! reading is one reader rather than one per pipeline.
//!
//! The matrices are a typed uniform block. Written once per frame, bound once,
//! read by all three programs from the same numbered point.
// ****************************************************************************
class MultiPassMesh: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "06_MultiPassMesh";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    struct Vertex
    {
        Vector3f position;
        Vector3f normal;
        Vector3f color;
    };

    // ------------------------------------------------------------------------
    //! \brief Build one of the three pipelines from its two stages and its
    //! state, after checking the vertex can feed it.
    // ------------------------------------------------------------------------
    [[nodiscard]] gpu::Status makePipeline(gpu::Program& p_program,
                                           gpu::Pipeline& p_pipeline,
                                           char const* p_vertex,
                                           char const* p_fragment,
                                           gpu::RenderState const& p_state);

    gpu::Program m_lit_program;
    gpu::Program m_normals_program;
    gpu::Program m_wire_program;

    gpu::Pipeline m_lit;
    gpu::Pipeline m_normals;
    gpu::Pipeline m_wire;

    gpu::Buffer<Vertex> m_vertices;
    gpu::Buffer<std::uint16_t> m_indices;
    gpu::TypedUniformBlock<Transforms> m_transforms;
};

} // namespace examples
