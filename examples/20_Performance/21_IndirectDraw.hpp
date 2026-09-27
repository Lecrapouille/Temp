//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
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
//! \code
//! m_command.write(gpu::DrawIndirectCommand{ 0u, 1u, 0u, 0u }, 0u);   // count = 0
//! COMPAGES_TRY(m_cull.dispatchItems(COUNT));        // counts the survivors
//! gpu::barrier(gpu::Barrier::VertexAttrib | gpu::Barrier::Command);
//! m_kept_points.drawIndirect(m_command);            // draws that many
//! \endcode
//!
//! 12 wrote particles and then drew all of them. Here the count itself is a
//! GPU result. Forgetting the command barrier is a race, the same way
//! forgetting the vertex barrier in 12 is.
// ****************************************************************************
class IndirectDraw: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "21_IndirectDraw";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    struct Dot
    {
        Vector2f position;
    };

    gpu::Buffer<Dot> m_all;
    gpu::Buffer<Dot> m_kept;
    gpu::Buffer<gpu::DrawIndirectCommand> m_command;
    gpu::ComputeProgram m_cull;
    gpu::Drawable m_kept_points;
};

} // namespace examples
