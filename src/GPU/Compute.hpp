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

#include "GPU/Buffer.hpp"
#include "GPU/Core/Enums.hpp"
#include "GPU/Core/Result.hpp"
#include "GPU/Shader.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>

// ****************************************************************************
//! \file
//! \brief Work that is not a picture: a compute program, a barrier, two buffers
//! that swap.
//!
//! A graphics program runs because a triangle covered a pixel. A compute
//! program runs because it was asked to, over a grid of work groups whose size
//! the shader itself declares. The same Buffer can be the storage block it
//! writes and the vertices a later draw reads: there is no conversion and no
//! copy, which is the reason 13_ComputeParticles exists.
//!
//! The device is allowed to run ahead. gpu::barrier() is what says the writes
//! are visible before the next pass reads them. Forgetting it is a race, not a
//! compile error.
// ****************************************************************************

namespace gpu
{

// ****************************************************************************
//! \brief A program with one compute stage, dispatched over a grid.
// ****************************************************************************
class ComputeProgram
{
public:

    ComputeProgram() = default;

    // ------------------------------------------------------------------------
    //! \brief Compile and link a compute program from source.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<ComputeProgram> fromSource(
        std::string_view p_source);

    // ------------------------------------------------------------------------
    //! \brief Compile and link a compute program from a file.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<ComputeProgram> fromFile(
        std::string const& p_path);

    ComputeProgram(ComputeProgram&&) noexcept = default;
    ComputeProgram& operator=(ComputeProgram&&) noexcept = default;
    ComputeProgram(ComputeProgram const&) = delete;
    ComputeProgram& operator=(ComputeProgram const&) = delete;

    [[nodiscard]] bool valid() const
    {
        return m_program.valid();
    }

    [[nodiscard]] Program& program()
    {
        return m_program;
    }

    [[nodiscard]] Program const& program() const
    {
        return m_program;
    }

    [[nodiscard]] ProgramReflection const& reflection() const
    {
        return m_program.reflection();
    }

    // ------------------------------------------------------------------------
    //! \brief The work group size the shader declared, or {0,0,0} if none.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::array<int, 3> const& workGroupSize() const
    {
        return m_program.reflection().work_group_size;
    }

    // ------------------------------------------------------------------------
    //! \brief Put a storage buffer on the binding point the named block reads.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status bind(std::string_view p_block, BufferHandle p_buffer);

    template <typename T>
    [[nodiscard]] Status bind(std::string_view p_block, Buffer<T> const& p_buffer)
    {
        return bind(p_block, p_buffer.handle());
    }

    // ------------------------------------------------------------------------
    //! \brief Set a uniform of the compute program.
    // ------------------------------------------------------------------------
    template <typename T>
    [[nodiscard]] Status set(std::string_view p_name, T const& p_value)
    {
        return m_program.set(p_name, p_value);
    }

    // ------------------------------------------------------------------------
    //! \brief Run over this many work groups on each axis.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status dispatch(std::uint32_t p_groups_x,
                                  std::uint32_t p_groups_y = 1u,
                                  std::uint32_t p_groups_z = 1u);

    // ------------------------------------------------------------------------
    //! \brief Run over enough groups to cover this many items on X.
    //!
    //! The shader's local_size_x is what one group covers. Asking for 1000
    //! items of a group of 64 launches 16 groups, and the shader is expected
    //! to return early past the last item.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status dispatchItems(std::uint32_t p_count);

private:

    explicit ComputeProgram(Program p_program) : m_program(std::move(p_program))
    {
    }

    Program m_program;
};

// ****************************************************************************
//! \brief Two storage buffers that swap roles each step.
//!
//! A compute pass cannot read and write the same element safely. PingPong holds
//! the previous state and the next one, and swap() is what makes this frame's
//! output next frame's input. Both are Storage, so either can be drawn as
//! vertices after a barrier.
// ****************************************************************************
template <typename T>
class PingPong
{
public:

    PingPong() = default;

    [[nodiscard]] static Result<PingPong> create(std::size_t p_count)
    {
        GPU_TRY_ASSIGN(first,
                       Buffer<T>::create(
                           p_count, BufferKind::Storage, BufferUsage::Storage));
        GPU_TRY_ASSIGN(second,
                       Buffer<T>::create(
                           p_count, BufferKind::Storage, BufferUsage::Storage));
        PingPong pair;
        pair.m_buffer[0] = std::move(first);
        pair.m_buffer[1] = std::move(second);
        pair.m_count = p_count;
        return pair;
    }

    [[nodiscard]] static Result<PingPong> from(std::span<const T> p_data)
    {
        GPU_TRY_ASSIGN(first,
                       Buffer<T>::from(
                           p_data, BufferKind::Storage, BufferUsage::Storage));
        GPU_TRY_ASSIGN(second,
                       Buffer<T>::from(
                           p_data, BufferKind::Storage, BufferUsage::Storage));
        PingPong pair;
        pair.m_buffer[0] = std::move(first);
        pair.m_buffer[1] = std::move(second);
        pair.m_count = p_data.size();
        return pair;
    }

    [[nodiscard]] Buffer<T>& input()
    {
        return m_buffer[m_current];
    }

    [[nodiscard]] Buffer<T> const& input() const
    {
        return m_buffer[m_current];
    }

    [[nodiscard]] Buffer<T>& output()
    {
        return m_buffer[1 - m_current];
    }

    [[nodiscard]] Buffer<T> const& output() const
    {
        return m_buffer[1 - m_current];
    }

    void swap()
    {
        m_current = 1 - m_current;
    }

    [[nodiscard]] std::size_t count() const
    {
        return m_count;
    }

    [[nodiscard]] bool valid() const
    {
        return m_buffer[0].valid() && m_buffer[1].valid();
    }

private:

    Buffer<T> m_buffer[2];
    int m_current = 0;
    std::size_t m_count = 0u;
};

// ----------------------------------------------------------------------------
//! \brief Make earlier GPU writes of these kinds visible to later reads.
// ----------------------------------------------------------------------------
void barrier(Barrier p_what);

} // namespace gpu
