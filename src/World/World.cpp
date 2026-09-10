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

#include "World/World.hpp"

#include <utility>

namespace world
{

const std::string World::s_empty_name{};

//------------------------------------------------------------------------------
Entity World::create(std::string p_name)
{
    Entity entity = m_registry.create();
    (void)m_graph.attach(entity);
    m_transforms.allocate(entity);
    if (!p_name.empty())
    {
        m_names.add(entity, std::move(p_name));
    }
    return entity;
}

//------------------------------------------------------------------------------
void World::destroy(Entity p_entity)
{
    if (!m_registry.alive(p_entity))
    {
        return;
    }
    destroyRecursive(p_entity);
}

//------------------------------------------------------------------------------
void World::destroyRecursive(Entity p_entity)
{
    // Walk children first so their components and transform slots are cleaned
    // up before their parent is.
    Entity child = firstChild(p_entity);
    while (child.valid())
    {
        const Entity next = nextSibling(child);
        destroyRecursive(child);
        child = next;
    }

    notifyEntityDestroyed(p_entity);
    m_graph.detach(p_entity);
    m_transforms.release(p_entity);
    m_registry.destroy(p_entity);
}

//------------------------------------------------------------------------------
void World::notifyEntityDestroyed(Entity p_entity)
{
    m_names.onEntityDestroyed(p_entity);
    m_disabled.onEntityDestroyed(p_entity);
    for (auto& kv : m_stores)
    {
        kv.second->onEntityDestroyed(p_entity);
    }
}

//------------------------------------------------------------------------------
bool World::alive(Entity p_entity) const
{
    return m_registry.alive(p_entity);
}

//------------------------------------------------------------------------------
std::string const& World::name(Entity p_entity) const
{
    if (auto const* value = m_names.tryGet(p_entity))
    {
        return *value;
    }
    return s_empty_name;
}

//------------------------------------------------------------------------------
void World::setName(Entity p_entity, std::string p_name)
{
    if (!alive(p_entity))
    {
        return;
    }
    if (p_name.empty())
    {
        m_names.remove(p_entity);
    }
    else
    {
        m_names.add(p_entity, std::move(p_name));
    }
}

//------------------------------------------------------------------------------
void World::setEnabled(Entity p_entity, bool p_enabled)
{
    if (!alive(p_entity))
    {
        return;
    }
    if (p_enabled)
    {
        m_disabled.remove(p_entity);
    }
    else
    {
        m_disabled.add(p_entity, 1u);
    }
}

//------------------------------------------------------------------------------
bool World::enabled(Entity p_entity) const
{
    if (!alive(p_entity))
    {
        return false;
    }
    return m_disabled.tryGet(p_entity) == nullptr;
}

//------------------------------------------------------------------------------
bool World::enabledInHierarchy(Entity p_entity) const
{
    Entity walk = p_entity;
    while (walk.valid())
    {
        if (!enabled(walk))
        {
            return false;
        }
        walk = parent(walk);
    }
    return true;
}

//------------------------------------------------------------------------------
gloop::Status World::setParent(Entity p_child,
                             Entity p_parent,
                             ReparentPolicy p_policy)
{
    if (!alive(p_child))
    {
        return gloop::failure(
            "setParent was given a child entity that no longer exists");
    }
    if (p_parent.valid() && !alive(p_parent))
    {
        return gloop::failure(
            "setParent was given a parent entity that no longer exists");
    }
    if (p_child == p_parent)
    {
        return gloop::failure("an entity cannot be its own parent");
    }

    const NodeId child = m_graph.nodeOf(p_child);
    const NodeId parent = p_parent.valid() ? m_graph.nodeOf(p_parent) : NodeId{};
    if (!child.valid())
    {
        return gloop::failure(
            "the child entity is not part of the spatial graph");
    }
    if (p_parent.valid() && !parent.valid())
    {
        return gloop::failure(
            "the parent entity is not part of the spatial graph");
    }

    return m_graph.setParent(child, parent, p_policy, &m_transforms);
}

//------------------------------------------------------------------------------
Entity World::parent(Entity p_entity) const
{
    const NodeId node = m_graph.nodeOf(p_entity);
    if (!node.valid())
    {
        return {};
    }
    return m_graph.entityOf(m_graph.parent(node));
}

//------------------------------------------------------------------------------
Entity World::firstChild(Entity p_entity) const
{
    const NodeId node = m_graph.nodeOf(p_entity);
    if (!node.valid())
    {
        return {};
    }
    return m_graph.entityOf(m_graph.firstChild(node));
}

//------------------------------------------------------------------------------
Entity World::nextSibling(Entity p_entity) const
{
    const NodeId node = m_graph.nodeOf(p_entity);
    if (!node.valid())
    {
        return {};
    }
    return m_graph.entityOf(m_graph.nextSibling(node));
}

//------------------------------------------------------------------------------
Entity World::find(Entity p_root, std::string_view p_path) const
{
    if (!alive(p_root))
    {
        return {};
    }

    Entity current = p_root;
    std::size_t begin = 0u;
    while (begin < p_path.size())
    {
        if (p_path[begin] == '/')
        {
            ++begin;
            continue;
        }
        const std::size_t slash = p_path.find('/', begin);
        const std::string_view part =
            p_path.substr(begin,
                          (slash == std::string_view::npos)
                              ? std::string_view::npos
                              : (slash - begin));

        Entity child = firstChild(current);
        Entity found;
        while (child.valid())
        {
            if (std::string_view(name(child)) == part)
            {
                found = child;
                break;
            }
            child = nextSibling(child);
        }
        if (!found.valid())
        {
            return {};
        }
        current = found;
        if (slash == std::string_view::npos)
        {
            break;
        }
        begin = slash + 1u;
    }
    return current;
}

//------------------------------------------------------------------------------
std::size_t World::descendantCount(Entity p_entity) const
{
    return m_graph.descendantCount(m_graph.nodeOf(p_entity));
}

//------------------------------------------------------------------------------
LocalTransform& World::transform(Entity p_entity)
{
    assert(alive(p_entity) && "World::transform on a stale entity");
    // Marking the entity dirty is enough: the update pass propagates dirt to
    // descendants as it walks. That is why we do not walk the subtree here.
    m_transforms.markDirty(p_entity);
    return m_transforms.localMutable(p_entity);
}

//------------------------------------------------------------------------------
LocalTransform const& World::transform(Entity p_entity) const
{
    assert(alive(p_entity) && "World::transform on a stale entity");
    return m_transforms.local(p_entity);
}

//------------------------------------------------------------------------------
Matrix44f const& World::worldMatrix(Entity p_entity) const
{
    assert(alive(p_entity) && "World::worldMatrix on a stale entity");
    return m_transforms.world(p_entity);
}

//------------------------------------------------------------------------------
void World::update()
{
    m_transform_system.update(m_graph, m_transforms);
}

} // namespace world
