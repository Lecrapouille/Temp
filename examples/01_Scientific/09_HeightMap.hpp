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
//! \brief A surface the CPU recomputes, a hundred thousand vertices at a time.
//!
//! This is what gpu::VertexArray is for. The height of every point is a
//! function of its place and of time, decided here, and update() sends the
//! whole array because the whole array changed. The triangles naming those
//! points do not move, so they live in a Buffer that is written once.
//!
//! 03_DynamicTriangle sent one vertex. This sends a hundred thousand. The
//! difference is the size of the dirty range, not the API.
// ****************************************************************************
class HeightMap: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "09_HeightMap";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    struct Vertex
    {
        Vector3f position;
        Vector3f normal;
    };

    gpu::Program m_program;
    gpu::Pipeline m_pipeline;
    gpu::VertexArray<Vertex> m_surface;
    gpu::Buffer<std::uint32_t> m_indices;
};

} // namespace examples
