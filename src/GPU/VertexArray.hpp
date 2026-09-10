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
#include "GPU/Core/DirtyRange.hpp"

#include <cassert>
#include <span>
#include <vector>

namespace gpu
{

// ****************************************************************************
//! \brief A std::vector that also lives on the device, sending over only what
//! has changed.
//!
//! Use this when the CPU is the one deciding what the data is, frame after
//! frame: an animated surface, a curve that grows a point per frame, geometry
//! rebuilt when a parameter moves. Use a plain gpu::Buffer instead when the data
//! is written once, and a compute pass when the device can work out the new
//! values by itself, since the fastest data is the data that never crosses the
//! bus at all.
//!
//! \code
//! gpu::VertexArray<Vertex> surface;
//! surface.resize(100000);
//!
//! // Once per frame: touch what moves, then send.
//! for (auto& vertex : surface.modify())
//!     vertex.position.z = height(vertex.position.x, vertex.position.y, time);
//! if (auto sent = surface.update(); !sent)
//!     std::cerr << sent.error() << std::endl;
//! \endcode
//!
//! Nothing here talks to the device except update(). The array can therefore be
//! filled before a window exists, and there is exactly one call to check the
//! result of, rather than one per element written.
//!
//! \tparam T what one element is, with the same requirement as gpu::Buffer: its
//! bytes go to the device as they are.
// ****************************************************************************
template <typename T>
class VertexArray
{
public:

    // ------------------------------------------------------------------------
    //! \brief An empty array.
    //! \param[in] p_kind what the memory will be used for. Indices need
    //! BufferKind::Index, and a compute pass reading it needs
    //! BufferKind::Storage.
    // ------------------------------------------------------------------------
    explicit VertexArray(BufferKind p_kind = BufferKind::Vertex)
        : m_kind(p_kind)
    {
    }

    // ------------------------------------------------------------------------
    //! \brief An array holding a copy of these elements, all of them waiting to
    //! be sent.
    // ------------------------------------------------------------------------
    explicit VertexArray(std::span<const T> p_data,
                         BufferKind p_kind = BufferKind::Vertex)
        : m_cpu(p_data.begin(), p_data.end()), m_kind(p_kind)
    {
        m_dirty.addAll(m_cpu.size());
    }

    // ------------------------------------------------------------------------
    //! \brief How many elements the array holds.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t size() const
    {
        return m_cpu.size();
    }

    // ------------------------------------------------------------------------
    //! \brief Does the array hold nothing?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool empty() const
    {
        return m_cpu.empty();
    }

    // ------------------------------------------------------------------------
    //! \brief Change how many elements the array holds.
    //!
    //! Growing marks the new elements to be sent. Shrinking sends nothing and
    //! leaves the memory on the device as it is: a draw call is told how many
    //! elements to read, so what lies beyond is never looked at.
    // ------------------------------------------------------------------------
    void resize(std::size_t p_count)
    {
        const std::size_t before = m_cpu.size();
        if (p_count == before)
        {
            return;
        }

        m_cpu.resize(p_count);
        if (p_count > before)
        {
            m_dirty.add(before, p_count - before);
        }
        else
        {
            // Whatever was waiting to be sent past the new end no longer exists.
            m_dirty.clampTo(p_count);
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Set aside room for that many elements without creating any.
    //!
    //! Worth doing when the final count is known: it keeps push_back() from
    //! moving the elements around, and it lets the first update() reserve the
    //! device memory once instead of growing it as the array fills.
    // ------------------------------------------------------------------------
    void reserve(std::size_t p_count)
    {
        m_cpu.reserve(p_count);
        m_reserved = std::max(m_reserved, p_count);
    }

    // ------------------------------------------------------------------------
    //! \brief Add one element at the end, marked to be sent.
    // ------------------------------------------------------------------------
    void push_back(T const& p_value)
    {
        m_cpu.push_back(p_value);
        m_dirty.add(m_cpu.size() - 1u);
    }

    // ------------------------------------------------------------------------
    //! \brief Throw the contents away, keeping the memory on the device for the
    //! next fill.
    // ------------------------------------------------------------------------
    void clear()
    {
        m_cpu.clear();
        m_dirty.clear();
    }

    // ------------------------------------------------------------------------
    //! \brief Read one element.
    // ------------------------------------------------------------------------
    [[nodiscard]] T const& operator[](std::size_t p_index) const
    {
        assert((p_index < m_cpu.size()) && "reading past the end of a VertexArray");
        return m_cpu[p_index];
    }

    // ------------------------------------------------------------------------
    //! \brief Reach one element in order to change it.
    //!
    //! Marks that element to be sent, whether or not it is really written: a
    //! reference that can be written has to be assumed to be. Read through a
    //! const array, or through the const operator[], to avoid that.
    // ------------------------------------------------------------------------
    [[nodiscard]] T& operator[](std::size_t p_index)
    {
        assert((p_index < m_cpu.size()) && "writing past the end of a VertexArray");
        m_dirty.add(p_index);
        return m_cpu[p_index];
    }

    // ------------------------------------------------------------------------
    //! \brief Overwrite one element.
    //!
    //! Says exactly what is meant, and cannot mark anything dirty by accident.
    // ------------------------------------------------------------------------
    void set(std::size_t p_index, T const& p_value)
    {
        assert((p_index < m_cpu.size()) && "writing past the end of a VertexArray");
        m_cpu[p_index] = p_value;
        m_dirty.add(p_index);
    }

    // ------------------------------------------------------------------------
    //! \brief The elements, to be walked over and changed in place.
    //!
    //! The whole array is marked to be sent, so this is what to use when most of
    //! it changes, as in a surface recomputed every frame. When only a small part
    //! moves, ask for that part instead.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::span<T> modify()
    {
        m_dirty.addAll(m_cpu.size());
        return { m_cpu.data(), m_cpu.size() };
    }

    // ------------------------------------------------------------------------
    //! \brief A run of elements, to be changed in place.
    //!
    //! \param[in] p_first index of the first one.
    //! \param[in] p_count how many.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::span<T> modify(std::size_t p_first, std::size_t p_count)
    {
        assert(((p_first + p_count) <= m_cpu.size()) &&
               "modifying past the end of a VertexArray");
        m_dirty.add(p_first, p_count);
        return { m_cpu.data() + p_first, p_count };
    }

    // ------------------------------------------------------------------------
    //! \brief The elements, read only, without marking anything to be sent.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::span<const T> elements() const
    {
        return { m_cpu.data(), m_cpu.size() };
    }

    // ------------------------------------------------------------------------
    //! \brief Replace the whole contents.
    // ------------------------------------------------------------------------
    void assign(std::span<const T> p_data)
    {
        m_cpu.assign(p_data.begin(), p_data.end());
        m_dirty.addAll(m_cpu.size());
    }

    // ------------------------------------------------------------------------
    //! \brief Send to the device whatever has changed since the last time.
    //!
    //! Reserves or enlarges the memory on the device when needed, then writes the
    //! one run of elements covering every change. Cheap to call when nothing has
    //! changed, so it belongs unconditionally in the frame loop.
    //!
    //! \return why the data could not be sent, which at this point means the
    //! device could not give us the memory.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status update()
    {
        if (m_cpu.empty())
        {
            m_dirty.clear();
            return success();
        }

        GPU_TRY(reserveDevice());

        if (m_dirty.empty())
        {
            return success();
        }

        // The two halves of what the previous layer got wrong, side by side: the
        // bytes are read from where the change is, and written at the offset the
        // change is at. Reading from the front of the container while writing at
        // an offset is what silently moved a vertex to the wrong place.
        const std::size_t first = m_dirty.begin();
        const std::size_t count = m_dirty.count();
        GPU_TRY(m_device.write(
            std::span<const T>(m_cpu.data() + first, count), first));

        m_dirty.clear();
        return success();
    }

    // ------------------------------------------------------------------------
    //! \brief Is anything waiting to be sent?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool dirty() const
    {
        return !m_dirty.empty();
    }

    // ------------------------------------------------------------------------
    //! \brief Which elements are waiting to be sent, as a half open range.
    // ------------------------------------------------------------------------
    [[nodiscard]] DirtyRange const& pending() const
    {
        return m_dirty;
    }

    // ------------------------------------------------------------------------
    //! \brief The memory on the device, to hand to a draw call.
    //!
    //! Empty until the first update(), since nothing is reserved before there is
    //! something to put in it.
    // ------------------------------------------------------------------------
    [[nodiscard]] Buffer<T> const& buffer() const
    {
        return m_device;
    }

    // ------------------------------------------------------------------------
    //! \brief Name of the memory on the device.
    // ------------------------------------------------------------------------
    [[nodiscard]] BufferHandle handle() const
    {
        return m_device.handle();
    }

    // ------------------------------------------------------------------------
    //! \brief What the memory is used for.
    // ------------------------------------------------------------------------
    [[nodiscard]] BufferKind kind() const
    {
        return m_kind;
    }

    // ------------------------------------------------------------------------
    //! \brief How many elements the device memory can hold, which is at least
    //! size() after an update() and may be more when the array is growing.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t deviceCapacity() const
    {
        return m_device.count();
    }

private:

    // ------------------------------------------------------------------------
    //! \brief Make sure the device has room for every element.
    //!
    //! Memory reserved on the device cannot be resized, so growing means asking
    //! for a new, larger block and sending everything again. Asking for more than
    //! is needed is what keeps a curve that gains a point per frame from doing
    //! that on every frame.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status reserveDevice()
    {
        if (m_device.valid() && (m_device.count() >= m_cpu.size()))
        {
            return success();
        }

        std::size_t capacity = std::max(m_cpu.size(), m_reserved);
        capacity = std::max(capacity, m_device.count() * 2u);

        GPU_TRY_ASSIGN(fresh,
                       Buffer<T>::create(capacity, m_kind, BufferUsage::Dynamic));
        m_device = std::move(fresh);

        // The new memory holds nothing, so what was already sent to the old one
        // counts as unsent again.
        m_dirty.addAll(m_cpu.size());
        return success();
    }

    //! \brief The copy the CPU works on.
    std::vector<T> m_cpu;
    //! \brief The copy the device draws from.
    Buffer<T> m_device;
    //! \brief What the two disagree on.
    DirtyRange m_dirty;
    //! \brief What the memory is for.
    BufferKind m_kind = BufferKind::Vertex;
    //! \brief How much room reserve() was asked for, so that the device memory
    //! is reserved once rather than doubled as the array fills.
    std::size_t m_reserved = 0u;
};

} // namespace gpu
