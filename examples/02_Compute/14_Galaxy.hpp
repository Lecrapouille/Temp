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
//! \brief N-body gravity, tiled shared memory, two buffers that swap.
//!
//! Particles bounced against walls they already knew. These stars pull on
//! every other star, which is why the work is done in tiles loaded into
//! shared memory: each work group reads a slice once and reuses it, the
//! way GPU Gems 3 taught.
//!
//! PingPong is two Storage buffers. This frame's output is next frame's
//! input. std430 is what lets the C++ Star and the GLSL Star occupy the
//! same bytes.
// ****************************************************************************
class Galaxy: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "14_Galaxy";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    struct Star
    {
        Vector2f position;
        Vector2f velocity;
        Vector4f color;
    };

    gpu::ComputeProgram m_step;
    gpu::Program m_draw_program;
    gpu::Pipeline m_draw;
    gpu::PingPong<Star> m_stars;
};

} // namespace examples
