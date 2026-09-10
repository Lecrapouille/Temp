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

#include "Common/Result.hpp"
#include "World/ComponentStore.hpp"
#include "World/Entity.hpp"
#include "World/EntityRegistry.hpp"
#include "World/SpatialGraph.hpp"
#include "World/TransformStore.hpp"
#include "World/TransformSystem.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>

namespace world
{

// ****************************************************************************
//! \brief The source of truth of a simulation.
//!
//! A World owns:
//! - the identity registry;
//! - the spatial graph;
//! - the transform store;
//! - one component store per component type used by the caller;
//! - a few first-class stores (name, enabled flag) that every entity may want.
//!
//! A World never touches \c gpu::. It never opens a window, never draws, never
//! resolves an asset id to a device resource. A World can run headless: the
//! same simulation code compiles and runs without a device present, which is
//! what a test, a tool or a server needs.
//!
//! Presentation is a separate concern. A \c scene::Scene references a World
//! and picks which of its entities are used for a given view. The renderer
//! reads a snapshot extracted from a World; it does not modify the World.
// ****************************************************************************
class World
{
public:

    World() = default;
    World(World const&) = delete;
    World& operator=(World const&) = delete;
    World(World&&) = default;
    World& operator=(World&&) = default;

    // ------------------------------------------------------------------------
    // Entities
    // ------------------------------------------------------------------------

    // ------------------------------------------------------------------------
    //! \brief Create an Entity, optionally with a name.
    //!
    //! The new Entity is spatial: a node is created for it in the graph and a
    //! transform slot is allocated in the store. That is the default a scene
    //! wants; an entity that stays out of the graph is rare enough to require
    //! calling \c detachSpatial() explicitly.
    // ------------------------------------------------------------------------
    [[nodiscard]] Entity create(std::string p_name = {});

    // ------------------------------------------------------------------------
    //! \brief Destroy an Entity and everything hanging from it.
    //!
    //! Destroys the spatial subtree first (children first), then the entity
    //! itself. Every component of every destroyed entity is dropped from its
    //! store.
    // ------------------------------------------------------------------------
    void destroy(Entity p_entity);

    [[nodiscard]] bool alive(Entity p_entity) const;

    [[nodiscard]] std::size_t living() const { return m_registry.living(); }

    // ------------------------------------------------------------------------
    //! \brief The entity's name, or an empty string.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string const& name(Entity p_entity) const;
    void setName(Entity p_entity, std::string p_name);

    // ------------------------------------------------------------------------
    //! \brief Enable/disable an entity, disabled entities being ignored by the
    //! renderer and by systems that check the flag.
    // ------------------------------------------------------------------------
    void setEnabled(Entity p_entity, bool p_enabled);
    [[nodiscard]] bool enabled(Entity p_entity) const;
    [[nodiscard]] bool enabledInHierarchy(Entity p_entity) const;

    // ------------------------------------------------------------------------
    // Spatial hierarchy
    // ------------------------------------------------------------------------

    [[nodiscard]] SpatialGraph& spatial() { return m_graph; }
    [[nodiscard]] SpatialGraph const& spatial() const { return m_graph; }

    // ------------------------------------------------------------------------
    //! \brief Change (or clear) the parent of an entity, addressed by Entity.
    //!
    //! Wraps \c SpatialGraph::setParent for callers who work in Entity terms.
    //! Refuses if either handle is stale or making the change would create a
    //! cycle.
    // ------------------------------------------------------------------------
    [[nodiscard]] gloop::Status
    setParent(Entity p_child,
              Entity p_parent,
              ReparentPolicy p_policy = ReparentPolicy::KeepLocal);

    [[nodiscard]] Entity parent(Entity p_entity) const;
    [[nodiscard]] Entity firstChild(Entity p_entity) const;
    [[nodiscard]] Entity nextSibling(Entity p_entity) const;

    // ------------------------------------------------------------------------
    //! \brief Walk children by name, the way the old robot demo found a leg.
    //!
    //! \param[in] p_root where to start.
    //! \param[in] p_path slash-separated names, e.g. \c "Body/LeftLeg".
    //! \return the matching entity or an empty handle when the path does not
    //! resolve.
    // ------------------------------------------------------------------------
    [[nodiscard]] Entity find(Entity p_root, std::string_view p_path) const;

    //! \brief How many entities hang under this one, itself included.
    [[nodiscard]] std::size_t descendantCount(Entity p_entity) const;

    // ------------------------------------------------------------------------
    // Transforms
    // ------------------------------------------------------------------------

    [[nodiscard]] TransformStore& transforms() { return m_transforms; }
    [[nodiscard]] TransformStore const& transforms() const
    {
        return m_transforms;
    }

    // ------------------------------------------------------------------------
    //! \brief The local pose. Writing to it marks the entity dirty; the update
    //! pass will recompute its world matrix and those of its descendants.
    // ------------------------------------------------------------------------
    [[nodiscard]] LocalTransform& transform(Entity p_entity);
    [[nodiscard]] LocalTransform const& transform(Entity p_entity) const;

    // ------------------------------------------------------------------------
    //! \brief The derived world matrix at the last \c update().
    // ------------------------------------------------------------------------
    [[nodiscard]] Matrix44f const& worldMatrix(Entity p_entity) const;

    // ------------------------------------------------------------------------
    //! \brief Recompute every dirty world matrix, parents before children.
    // ------------------------------------------------------------------------
    void update();

    // ------------------------------------------------------------------------
    // Components (generic)
    // ------------------------------------------------------------------------

    // ------------------------------------------------------------------------
    //! \brief The component store for type \c T, created on first access.
    // ------------------------------------------------------------------------
    template <typename T>
    [[nodiscard]] ComponentStore<T>& components()
    {
        return storeFor<T>().store;
    }

    template <typename T>
    [[nodiscard]] ComponentStore<T> const& components() const
    {
        return const_cast<World*>(this)->storeFor<T>().store;
    }

    // ------------------------------------------------------------------------
    //! \brief Attach a component to an entity, replacing what was there.
    // ------------------------------------------------------------------------
    template <typename T>
    T& add(Entity p_entity, T p_component = T{})
    {
        assert(alive(p_entity) && "World::add on a stale entity");
        return storeFor<T>().store.add(p_entity, std::move(p_component));
    }

    // ------------------------------------------------------------------------
    //! \brief Detach a component from an entity.
    // ------------------------------------------------------------------------
    template <typename T>
    void remove(Entity p_entity)
    {
        storeFor<T>().store.remove(p_entity);
    }

    template <typename T>
    [[nodiscard]] bool has(Entity p_entity) const
    {
        auto it = m_stores.find(std::type_index(typeid(T)));
        if (it == m_stores.end())
        {
            return false;
        }
        return static_cast<TypedComponentStore<T>*>(it->second.get())
            ->store.has(p_entity);
    }

    template <typename T>
    [[nodiscard]] T* tryGet(Entity p_entity)
    {
        auto it = m_stores.find(std::type_index(typeid(T)));
        if (it == m_stores.end())
        {
            return nullptr;
        }
        return static_cast<TypedComponentStore<T>*>(it->second.get())
            ->store.tryGet(p_entity);
    }

    template <typename T>
    [[nodiscard]] T const* tryGet(Entity p_entity) const
    {
        auto it = m_stores.find(std::type_index(typeid(T)));
        if (it == m_stores.end())
        {
            return nullptr;
        }
        return static_cast<TypedComponentStore<T> const*>(it->second.get())
            ->store.tryGet(p_entity);
    }

    template <typename T>
    [[nodiscard]] T& get(Entity p_entity)
    {
        T* value = tryGet<T>(p_entity);
        assert(value != nullptr && "World::get on an entity without one");
        return *value;
    }

private:

    template <typename T>
    TypedComponentStore<T>& storeFor()
    {
        const std::type_index key(typeid(T));
        auto it = m_stores.find(key);
        if (it == m_stores.end())
        {
            auto holder = std::make_unique<TypedComponentStore<T>>();
            TypedComponentStore<T>* raw = holder.get();
            m_stores.emplace(key, std::move(holder));
            return *raw;
        }
        return *static_cast<TypedComponentStore<T>*>(it->second.get());
    }

    void notifyEntityDestroyed(Entity p_entity);
    void destroyRecursive(Entity p_entity);

    EntityRegistry m_registry;
    SpatialGraph m_graph;
    TransformStore m_transforms;
    TransformSystem m_transform_system;

    //! \brief Names live in a component store too, which keeps the World's own
    //! layout uniform and lets a caller drop the name of an entity if none is
    //! needed.
    ComponentStore<std::string> m_names;
    //! \brief Enabled flag. Missing means "enabled": paying for a flag on every
    //! entity that never disables anything is what the previous prototype did.
    ComponentStore<std::uint8_t> m_disabled;

    std::unordered_map<std::type_index, std::unique_ptr<IComponentStore>>
        m_stores;

    static const std::string s_empty_name;
};

} // namespace world
