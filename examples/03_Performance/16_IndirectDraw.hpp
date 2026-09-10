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
//! \brief The CPU never asks how many points survived.
//!
//! A compute pass keeps the points that fall inside a circle and writes the
//! four words a drawIndirect command is. The draw reads that command from the
//! device: there is no read-back, and no count to keep in step on the CPU.
//!
//! Thirteen wrote particles and then drew all of them. Here the count itself
//! is a GPU result. Forgetting the command barrier is a race, the same way
//! forgetting the vertex barrier in 13 is.
// ****************************************************************************
class IndirectDraw: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "16_IndirectDraw";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    struct Dot
    {
        Vector2f position;
    };

    gpu::ComputeProgram m_cull;
    gpu::Program m_draw_program;
    gpu::Pipeline m_draw;
    gpu::Buffer<Dot> m_all;
    gpu::Buffer<Dot> m_kept;
    gpu::Buffer<gpu::DrawIndirectCommand> m_command;
    std::uint32_t m_count = 0u;
};

} // namespace examples
