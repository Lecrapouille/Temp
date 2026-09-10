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
//! \brief A texture read by a shader, and a strip instead of separate triangles.
//!
//! The texture is computed here rather than read from a file, so that the example
//! depends on nothing outside itself and so that its content is knowable: a
//! checkerboard, with a gradient over it. Reading a file is one call away, and
//! Texture::fromFile() is what 09_HeightMap uses.
//!
//! Two other things are on show. A sampler is set like any other uniform, by
//! giving it the number of the unit the texture was bound to, and getting that
//! wrong is one of the errors the library reports rather than drawing black. And
//! the four corners are drawn as a triangle strip: the primitive belongs to the
//! render state, so it is part of what the pipeline was validated for, not an
//! argument passed to every draw.
// ****************************************************************************
class TexturedQuad: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "04_TexturedQuad";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    struct Vertex
    {
        Vector2f position;
        Vector2f uv;
    };

    // ------------------------------------------------------------------------
    //! \brief Build the checkerboard the example shows.
    // ------------------------------------------------------------------------
    [[nodiscard]] gpu::Status makeTexture();

    gpu::Program m_program;
    gpu::Pipeline m_pipeline;
    gpu::Buffer<Vertex> m_vertices;
    gpu::Texture m_texture;
};

} // namespace examples
