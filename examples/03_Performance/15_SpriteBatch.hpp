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
//! \brief A hundred thousand sprites, one draw call.
//!
//! The corners of a quad are four vertices. A hundred thousand quads used to
//! be a hundred thousand draws, or a buffer of four hundred thousand vertices
//! rebuilt whenever one sprite moved. Instancing is the third way: the corners
//! are generated from gl_VertexID, and the buffer holds one record per sprite,
//! read once per object because every field is marked perInstance().
//!
//! The library has a single vertex binding. Mixing per-vertex corners with
//! per-instance data in one layout is therefore not something it can say. A
//! generated quad plus an instance-only buffer is the shape that fits.
//!
//! The atlas is four by four tiles computed here. Each sprite names a tile;
//! the fragment shader reads it. One texture, one draw, a hundred thousand
//! quads.
// ****************************************************************************
class SpriteBatch: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "15_SpriteBatch";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    struct Sprite
    {
        Vector2f center;
        Vector2f extent;
        float tile = 0.0f;
        float phase = 0.0f;
    };

    [[nodiscard]] gpu::Status makeAtlas();

    gpu::Program m_program;
    gpu::Pipeline m_pipeline;
    gpu::Buffer<Sprite> m_sprites;
    gpu::Texture m_atlas;
    std::size_t m_count = 0u;
};

} // namespace examples
