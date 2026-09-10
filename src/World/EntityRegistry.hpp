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

#include "World/Entity.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace world
{

// ****************************************************************************
//! \brief The identity source of a World.
//!
//! The registry owns the mapping between Entities and the slots that back them.
//! It does not know about components, transforms, hierarchy or rendering: those
//! things live in their own stores and reference an Entity by its handle.
//!
//! A slot is a small record made of one std::uint16_t generation counter and a
//! free list. Creating an entity picks a slot; destroying it bumps the slot's
//! generation and pushes the slot back on the free list. That is what makes a
//! handle stale rather than dangling: an entity created on a reused slot has a
//! different generation than the one that used to live there, so a leftover
//! handle no longer names it.
//!
//! Slot zero is a live slot like any other. It is generation zero which is
//! reserved for empty Entity handles, so the very first entity has index 0
//! and generation 1.
// ****************************************************************************
class EntityRegistry
{
public:

    EntityRegistry() = default;
    EntityRegistry(EntityRegistry const&) = delete;
    EntityRegistry& operator=(EntityRegistry const&) = delete;
    EntityRegistry(EntityRegistry&&) = default;
    EntityRegistry& operator=(EntityRegistry&&) = default;

    // ------------------------------------------------------------------------
    //! \brief Make a new Entity.
    //!
    //! Reuses a freed slot when one is available. When none is, a new slot is
    //! appended: the registry never shrinks by itself, so a long-running World
    //! that spikes and then quiets down keeps the capacity it reached.
    //!
    //! \return the new Entity, with a generation that has never been used on
    //! this slot before.
    // ------------------------------------------------------------------------
    [[nodiscard]] Entity create();

    // ------------------------------------------------------------------------
    //! \brief Destroy an Entity and free its slot.
    //!
    //! Bumps the slot's generation, so any handle held elsewhere becomes stale
    //! and \c alive() will report it as such. Passing a stale or empty handle
    //! is a no-op: destruction is idempotent, which is what a shutdown pass
    //! walking a list of handles needs.
    // ------------------------------------------------------------------------
    void destroy(Entity p_entity);

    // ------------------------------------------------------------------------
    //! \brief Is this Entity still alive?
    //!
    //! True when the handle is not empty and the slot's generation still
    //! matches the handle's. False for an empty handle, a handle that named
    //! something already destroyed, or a handle from another registry.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool alive(Entity p_entity) const;

    // ------------------------------------------------------------------------
    //! \brief How many entities are alive right now.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t living() const
    {
        return m_living;
    }

    // ------------------------------------------------------------------------
    //! \brief How many slots have ever been used, alive or free.
    //!
    //! Useful to size a parallel array indexed by \c Entity::index(): every
    //! index that has ever named an entity is strictly less than this.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t capacity() const
    {
        return m_generation.size();
    }

    // ------------------------------------------------------------------------
    //! \brief The generation currently held by a slot.
    //!
    //! Component stores use this to keep their sparse arrays in step: an entry
    //! whose generation does not match this one is a leftover from a previous
    //! use of the slot and can be ignored.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::uint16_t generationAt(std::size_t p_index) const
    {
        return (p_index < m_generation.size()) ? m_generation[p_index] : 0u;
    }

    // ------------------------------------------------------------------------
    //! \brief Walk every living Entity, in slot order.
    //!
    //! \tparam F a callable receiving one \c Entity by value.
    // ------------------------------------------------------------------------
    template <typename F>
    void forEach(F&& p_callback) const
    {
        for (std::size_t i = 0u; i < m_generation.size(); ++i)
        {
            const std::uint16_t generation = m_generation[i];
            if ((generation != 0u) && !m_slot_free[i])
            {
                p_callback(Entity(static_cast<std::uint16_t>(i), generation));
            }
        }
    }

private:

    //! \brief One generation per slot. Zero is the sentinel for "never used";
    //! live slots hold a generation in [1, 0xFFFF].
    std::vector<std::uint16_t> m_generation;
    //! \brief Freed slot indices ready to be reused.
    std::vector<std::uint16_t> m_free;
    //! \brief Whether each slot is currently on the free list. A parallel array
    //! rather than a search of \c m_free, so \c alive() and \c forEach() do not
    //! degrade with the number of freed slots.
    std::vector<std::uint8_t> m_slot_free;
    //! \brief How many entities are currently alive.
    std::size_t m_living = 0u;
};

} // namespace world
