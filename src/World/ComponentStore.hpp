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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace world
{

// ****************************************************************************
//! \brief A sparse set holding one component of type T for a subset of the
//! Entities of a World.
//!
//! Two parallel arrays back the store: a sparse array indexed by
//! \c Entity::index() that says at which dense slot an entity's component
//! lives, and a pair of dense arrays holding the entities in insertion order
//! and their components side by side.
//!
//! Adding, removing and looking up a component is O(1). Walking the store is a
//! straight pass over the dense arrays, which is what a system consuming a
//! component wants: no gap, no null check, no random access.
//!
//! Removal uses the swap-with-last trick, which keeps the dense arrays packed
//! and does not invalidate any handle held elsewhere: only the last entity of
//! the store moves, and its sparse entry is updated.
//!
//! The store does not own the Entities. It reacts to their destruction through
//! \c onEntityDestroyed() when the World tells it to.
//!
//! \tparam T the component. Must be movable.
// ****************************************************************************
template <typename T>
class ComponentStore
{
public:

    //! \brief What "no dense slot" looks like inside the sparse array.
    static constexpr std::uint32_t INVALID_INDEX =
        static_cast<std::uint32_t>(-1);

    ComponentStore() = default;

    // ------------------------------------------------------------------------
    //! \brief Attach a component to an Entity, or replace the one already there.
    //!
    //! Passing a stale handle is a programming mistake: it will be caught by
    //! the assertion in the caller (typically \c World::add). This function
    //! only asks for a valid handle.
    //!
    //! \param[in] p_entity the entity the component is for. Must be non-empty.
    //! \param[in] p_component the value to store. Moved from.
    //! \return a reference to the stored component.
    // ------------------------------------------------------------------------
    T& add(Entity p_entity, T p_component = T{})
    {
        assert(p_entity.valid() && "ComponentStore::add on an empty entity");
        const std::size_t sparse_index = p_entity.index();
        ensureSparse(sparse_index);

        std::uint32_t dense = m_sparse[sparse_index];
        if ((dense != INVALID_INDEX) &&
            (m_dense_entity[dense] == p_entity))
        {
            m_dense_component[dense] = std::move(p_component);
            return m_dense_component[dense];
        }

        // The slot may have been reused: a stale entry left over from a
        // previous entity that shared the slot. Drop it before writing the new
        // component in.
        if (dense != INVALID_INDEX)
        {
            m_sparse[sparse_index] = INVALID_INDEX;
        }

        const auto new_dense = static_cast<std::uint32_t>(m_dense_entity.size());
        m_dense_entity.push_back(p_entity);
        m_dense_component.push_back(std::move(p_component));
        m_sparse[sparse_index] = new_dense;
        return m_dense_component.back();
    }

    // ------------------------------------------------------------------------
    //! \brief Detach a component from an Entity. A no-op when there is none.
    // ------------------------------------------------------------------------
    void remove(Entity p_entity)
    {
        if (!has(p_entity))
        {
            return;
        }
        removeAt(p_entity.index(), p_entity);
    }

    // ------------------------------------------------------------------------
    //! \brief Called by the World when an Entity is destroyed.
    //!
    //! Drops the component if one was there for that entity. Uses only the
    //! entity's index: the handle's generation may already be out of date at
    //! this point, and the store has to clean up whatever is there.
    // ------------------------------------------------------------------------
    void onEntityDestroyed(Entity p_entity)
    {
        if (!p_entity.valid())
        {
            return;
        }
        const std::size_t sparse_index = p_entity.index();
        if (sparse_index >= m_sparse.size())
        {
            return;
        }
        const std::uint32_t dense = m_sparse[sparse_index];
        if (dense == INVALID_INDEX)
        {
            return;
        }
        removeAt(sparse_index, m_dense_entity[dense]);
    }

    // ------------------------------------------------------------------------
    //! \brief Is there a component attached to this Entity?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool has(Entity p_entity) const
    {
        if (!p_entity.valid())
        {
            return false;
        }
        const std::size_t sparse_index = p_entity.index();
        if (sparse_index >= m_sparse.size())
        {
            return false;
        }
        const std::uint32_t dense = m_sparse[sparse_index];
        if (dense == INVALID_INDEX)
        {
            return false;
        }
        return m_dense_entity[dense] == p_entity;
    }

    // ------------------------------------------------------------------------
    //! \brief The component attached to an Entity, or nullptr when there is
    //! none.
    // ------------------------------------------------------------------------
    [[nodiscard]] T* tryGet(Entity p_entity)
    {
        if (!has(p_entity))
        {
            return nullptr;
        }
        return &m_dense_component[m_sparse[p_entity.index()]];
    }

    [[nodiscard]] T const* tryGet(Entity p_entity) const
    {
        if (!has(p_entity))
        {
            return nullptr;
        }
        return &m_dense_component[m_sparse[p_entity.index()]];
    }

    // ------------------------------------------------------------------------
    //! \brief The component attached to an Entity. Asserts when there is none.
    // ------------------------------------------------------------------------
    [[nodiscard]] T& get(Entity p_entity)
    {
        assert(has(p_entity) && "ComponentStore::get on an entity without one");
        return m_dense_component[m_sparse[p_entity.index()]];
    }

    [[nodiscard]] T const& get(Entity p_entity) const
    {
        assert(has(p_entity) && "ComponentStore::get on an entity without one");
        return m_dense_component[m_sparse[p_entity.index()]];
    }

    // ------------------------------------------------------------------------
    //! \brief How many components are stored.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t size() const
    {
        return m_dense_entity.size();
    }

    [[nodiscard]] bool empty() const
    {
        return m_dense_entity.empty();
    }

    // ------------------------------------------------------------------------
    //! \brief The Entities that have a component in this store, packed.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::span<Entity const> entities() const
    {
        return { m_dense_entity.data(), m_dense_entity.size() };
    }

    // ------------------------------------------------------------------------
    //! \brief The components themselves, packed and in the same order as
    //! \c entities().
    // ------------------------------------------------------------------------
    [[nodiscard]] std::span<T> components()
    {
        return { m_dense_component.data(), m_dense_component.size() };
    }

    [[nodiscard]] std::span<T const> components() const
    {
        return { m_dense_component.data(), m_dense_component.size() };
    }

    // ------------------------------------------------------------------------
    //! \brief Drop everything without releasing capacity.
    // ------------------------------------------------------------------------
    void clear()
    {
        for (auto& slot : m_sparse)
        {
            slot = INVALID_INDEX;
        }
        m_dense_entity.clear();
        m_dense_component.clear();
    }

private:

    void ensureSparse(std::size_t p_index)
    {
        if (p_index >= m_sparse.size())
        {
            m_sparse.resize(p_index + 1u, INVALID_INDEX);
        }
    }

    void removeAt(std::size_t p_sparse_index, Entity p_expected_entity)
    {
        if (p_sparse_index >= m_sparse.size())
        {
            return;
        }
        const std::uint32_t dense = m_sparse[p_sparse_index];
        assert(dense != INVALID_INDEX);
        assert(m_dense_entity[dense] == p_expected_entity);
        (void)p_expected_entity;

        const std::uint32_t last =
            static_cast<std::uint32_t>(m_dense_entity.size() - 1u);
        if (dense != last)
        {
            m_dense_entity[dense] = m_dense_entity[last];
            m_dense_component[dense] = std::move(m_dense_component[last]);
            m_sparse[m_dense_entity[dense].index()] = dense;
        }
        m_dense_entity.pop_back();
        m_dense_component.pop_back();
        m_sparse[p_sparse_index] = INVALID_INDEX;
    }

    std::vector<std::uint32_t> m_sparse;
    std::vector<Entity> m_dense_entity;
    std::vector<T> m_dense_component;
};

// ****************************************************************************
//! \brief Type-erased base of a component store, so a World can own stores of
//! any component type in one map and forward \c onEntityDestroyed() to them.
// ****************************************************************************
class IComponentStore
{
public:
    virtual ~IComponentStore() = default;
    virtual void onEntityDestroyed(Entity p_entity) = 0;
};

// ****************************************************************************
//! \brief Adapter making a \c ComponentStore<T> look like an \c IComponentStore.
// ****************************************************************************
template <typename T>
class TypedComponentStore final : public IComponentStore
{
public:
    void onEntityDestroyed(Entity p_entity) override
    {
        store.onEntityDestroyed(p_entity);
    }

    ComponentStore<T> store;
};

} // namespace world
