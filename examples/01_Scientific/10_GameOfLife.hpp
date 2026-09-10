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
//! \brief Compute in a texture, by drawing into another.
//!
//! Each cell is a pixel. A fullscreen pass reads the current generation and
//! writes the next into a second texture. The two swap, which is why this is
//! called ping-pong: a shader cannot read the picture it is writing, so the
//! next generation has to live somewhere else.
//!
//! This is the historical way glumpy computed on the GPU, and the reason 07
//! exists. A compute shader will do the same job with less ceremony in
//! 13_ComputeParticles; this one is the approach that works with nothing but
//! a framebuffer.
// ****************************************************************************
class GameOfLife: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "10_GameOfLife";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    [[nodiscard]] gpu::Status makeField(gpu::Texture& p_texture,
                                        gpu::Framebuffer& p_target);
    [[nodiscard]] gpu::Status seed();

    gpu::Program m_step_program;
    gpu::Pipeline m_step;
    gpu::Program m_show_program;
    gpu::Pipeline m_show;

    gpu::Texture m_field[2];
    gpu::Framebuffer m_target[2];
    int m_current = 0;
};

} // namespace examples
