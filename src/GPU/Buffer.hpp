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

#include "GPU/Core/Enums.hpp"
#include "GPU/Core/Handle.hpp"
#include "GPU/Core/Result.hpp"

#include <cstddef>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

namespace gpu
{

//! \brief Names memory living on the device.
using BufferHandle = Handle<struct BufferTag>;

namespace detail
{

// ----------------------------------------------------------------------------
// The work of a buffer, done once in bytes rather than once per element type.
// A template that only converts counts into bytes keeps the code the compiler
// has to stamp out for every vertex struct down to almost nothing.
// ----------------------------------------------------------------------------

//! \brief Reserve memory on the device. See gpu::Buffer::create().
[[nodiscard]] Result<BufferHandle> createBuffer(std::size_t p_bytes,
                                                const void* p_data,
                                                BufferKind p_kind,
                                                BufferUsage p_usage);

//! \brief Give the memory back. Does nothing on a handle already released.
void destroyBuffer(BufferHandle p_handle);

//! \brief Overwrite part of a buffer, checking the range and the usage first.
[[nodiscard]] Status writeBuffer(BufferHandle p_handle,
                                 std::size_t p_offset,
                                 std::size_t p_bytes,
                                 const void* p_data);

//! \brief Read part of a buffer back.
[[nodiscard]] Status readBuffer(BufferHandle p_handle,
                                std::size_t p_offset,
                                std::size_t p_bytes,
                                void* p_data);

//! \brief Size in bytes, or 0 when the handle is stale.
[[nodiscard]] std::size_t bufferBytes(BufferHandle p_handle);

//! \brief Is the buffer still alive?
[[nodiscard]] bool bufferAlive(BufferHandle p_handle);

//! \brief What the buffer is for.
[[nodiscard]] BufferKind bufferKind(BufferHandle p_handle);

//! \brief How often the buffer may be written.
[[nodiscard]] BufferUsage bufferUsage(BufferHandle p_handle);

//! \brief How many buffers are alive. Used to spot leaks.
[[nodiscard]] std::size_t liveBufferCount();

//! \brief How many bytes of buffers are held, as they were asked for.
[[nodiscard]] std::size_t bufferMemory();

} // namespace detail

// ****************************************************************************
//! \brief A block of memory on the device, holding a known number of a known
//! type.
//!
//! One class covers what the previous layer spread over separate types for
//! vertex data, indices and uniforms: what changes between them is the kind
//! given at creation, not the code. It also covers what the previous layer could
//! not express at all, since it insisted on one buffer per vertex attribute:
//! here a Buffer<Vertex> holds whole interleaved vertices.
//!
//! \code
//! struct Star { Vector3f position; Vector3f velocity; };
//!
//! auto buffer = gpu::Buffer<Star>::create(1000000, gpu::BufferKind::Storage,
//!                                        gpu::BufferUsage::Storage);
//! if (!buffer) { std::cerr << buffer.error(); return; }
//! buffer.value().write(stars);
//! \endcode
//!
//! The memory is released when the Buffer is destroyed. A Buffer can be moved but
//! not copied, since two owners of the same memory would release it twice.
//!
//! \tparam T what one element is. Must be trivially copyable: the bytes are sent
//! to the device as they are, and a type owning memory elsewhere would send a
//! pointer the device cannot follow.
// ****************************************************************************
template <typename T>
class Buffer
{
public:

    static_assert(std::is_trivially_copyable_v<T>,
                  "a buffer sends the bytes of T straight to the device, so T "
                  "must be trivially copyable. A type holding a pointer, a "
                  "std::string or a std::vector would send an address the "
                  "device cannot follow");

    // ------------------------------------------------------------------------
    //! \brief An empty buffer, owning nothing.
    // ------------------------------------------------------------------------
    Buffer() = default;

    // ------------------------------------------------------------------------
    //! \brief Reserve room for a number of elements, without filling it.
    //!
    //! \param[in] p_count how many elements. Not zero: a buffer of nothing is
    //! almost always a count that was computed wrong.
    //! \param[in] p_kind what the buffer is for.
    //! \param[in] p_usage how often it will be written from the CPU. Immutable is
    //! refused here, since an immutable buffer can never be filled.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Buffer> create(std::size_t p_count,
                                               BufferKind p_kind,
                                               BufferUsage p_usage)
    {
        if (p_usage == BufferUsage::Immutable)
        {
            return failure(
                "an immutable buffer can never be written, so it must be given "
                "its contents when created. Use Buffer::from() with the data, "
                "or ask for BufferUsage::Dynamic");
        }
        GPU_TRY_ASSIGN(handle,
                       detail::createBuffer(
                           p_count * sizeof(T), nullptr, p_kind, p_usage));
        return Buffer(handle, p_count);
    }

    // ------------------------------------------------------------------------
    //! \brief Reserve room and fill it in one go.
    //!
    //! The only way to make an immutable buffer, and the right one for geometry
    //! read from a file: the driver may then place it in the memory the device
    //! reads fastest, knowing it will never be written again.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Buffer> from(std::span<const T> p_data,
                                             BufferKind p_kind,
                                             BufferUsage p_usage)
    {
        GPU_TRY_ASSIGN(handle,
                       detail::createBuffer(p_data.size_bytes(),
                                            p_data.data(),
                                            p_kind,
                                            p_usage));
        return Buffer(handle, p_data.size());
    }

    // ------------------------------------------------------------------------
    //! \brief Hand the memory over to another Buffer.
    // ------------------------------------------------------------------------
    Buffer(Buffer&& p_other) noexcept
        : m_handle(p_other.m_handle), m_count(p_other.m_count)
    {
        p_other.m_handle = BufferHandle{};
        p_other.m_count = 0u;
    }

    // ------------------------------------------------------------------------
    //! \brief Take over another Buffer's memory, releasing our own first.
    // ------------------------------------------------------------------------
    Buffer& operator=(Buffer&& p_other) noexcept
    {
        if (this != &p_other)
        {
            release();
            m_handle = p_other.m_handle;
            m_count = p_other.m_count;
            p_other.m_handle = BufferHandle{};
            p_other.m_count = 0u;
        }
        return *this;
    }

    Buffer(Buffer const&) = delete;
    Buffer& operator=(Buffer const&) = delete;

    // ------------------------------------------------------------------------
    //! \brief Release the memory.
    // ------------------------------------------------------------------------
    ~Buffer()
    {
        release();
    }

    // ------------------------------------------------------------------------
    //! \brief Give the memory back now rather than at the end of the scope.
    // ------------------------------------------------------------------------
    void release()
    {
        detail::destroyBuffer(m_handle);
        m_handle = BufferHandle{};
        m_count = 0u;
    }

    // ------------------------------------------------------------------------
    //! \brief Overwrite the beginning of the buffer.
    //!
    //! \param[in] p_data the elements to write. Must fit.
    //! \param[in] p_first which element to start at.
    //! \return why it could not be written: too much data, or a buffer created
    //! immutable.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status write(std::span<const T> p_data,
                               std::size_t p_first = 0u)
    {
        return detail::writeBuffer(m_handle,
                                   p_first * sizeof(T),
                                   p_data.size_bytes(),
                                   p_data.data());
    }

    // ------------------------------------------------------------------------
    //! \brief Overwrite one element.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status write(T const& p_value, std::size_t p_index)
    {
        return detail::writeBuffer(
            m_handle, p_index * sizeof(T), sizeof(T), &p_value);
    }

    // ------------------------------------------------------------------------
    //! \brief Read elements back into CPU memory.
    //!
    //! Waits for the device to finish with the memory, so it belongs in tests and
    //! in debugging rather than in a frame. A compute pass that needs its result
    //! on the CPU every frame wants a second buffer and one frame of delay
    //! instead.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status read(std::span<T> p_into, std::size_t p_first = 0u) const
    {
        return detail::readBuffer(m_handle,
                                  p_first * sizeof(T),
                                  p_into.size_bytes(),
                                  p_into.data());
    }

    // ------------------------------------------------------------------------
    //! \brief Read the whole buffer back into a vector.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<std::vector<T>> read() const
    {
        std::vector<T> out(m_count);
        GPU_TRY(detail::readBuffer(
            m_handle, 0u, out.size() * sizeof(T), out.data()));
        return out;
    }

    // ------------------------------------------------------------------------
    //! \brief How many elements the buffer holds.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t count() const
    {
        return m_count;
    }

    // ------------------------------------------------------------------------
    //! \brief How many bytes the buffer occupies on the device.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t bytes() const
    {
        return m_count * sizeof(T);
    }

    // ------------------------------------------------------------------------
    //! \brief Does this Buffer own anything?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool valid() const
    {
        return detail::bufferAlive(m_handle);
    }

    // ------------------------------------------------------------------------
    //! \brief What the buffer is for.
    // ------------------------------------------------------------------------
    [[nodiscard]] BufferKind kind() const
    {
        return detail::bufferKind(m_handle);
    }

    // ------------------------------------------------------------------------
    //! \brief How often the buffer may be written.
    // ------------------------------------------------------------------------
    [[nodiscard]] BufferUsage usage() const
    {
        return detail::bufferUsage(m_handle);
    }

    // ------------------------------------------------------------------------
    //! \brief The handle naming this memory, for the parts of the library that
    //! take one, such as a draw call.
    // ------------------------------------------------------------------------
    [[nodiscard]] BufferHandle handle() const
    {
        return m_handle;
    }

private:

    Buffer(BufferHandle p_handle, std::size_t p_count)
        : m_handle(p_handle), m_count(p_count)
    {
    }

    BufferHandle m_handle;
    std::size_t m_count = 0u;
};

// ----------------------------------------------------------------------------
//! \brief How many buffers are alive right now.
//!
//! What the examples gallery watches: a demo that has been closed and still
//! leaves buffers behind is leaking them.
// ----------------------------------------------------------------------------
[[nodiscard]] inline std::size_t liveBuffers()
{
    return detail::liveBufferCount();
}

} // namespace gpu
