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
//! \brief The same buffer, written by a compute pass and drawn as points.
//!
//! Game of Life computed in a texture because a fragment shader can only write
//! the pixel it covers. A compute shader writes wherever it wants, so the
//! particles live in a Buffer that is a storage block for one pass and a
//! vertex buffer for the next. There is no copy and no conversion.
//!
//! gpu::barrier() is what makes the writes visible before the draw. Forgetting
//! it is a race, not a compile error.
// ****************************************************************************
class ComputeParticles: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "13_ComputeParticles";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    struct Particle
    {
        Vector2f position;
        Vector2f velocity;
        Vector4f color;
    };

    gpu::ComputeProgram m_step;
    gpu::Program m_draw_program;
    gpu::Pipeline m_draw;
    gpu::Buffer<Particle> m_particles;
};

} // namespace examples
