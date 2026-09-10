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

#include "World/EntityRegistry.hpp"

namespace world
{

//------------------------------------------------------------------------------
Entity EntityRegistry::create()
{
    std::uint16_t index = 0u;
    if (!m_free.empty())
    {
        index = m_free.back();
        m_free.pop_back();
        m_slot_free[index] = 0u;
    }
    else
    {
        index = static_cast<std::uint16_t>(m_generation.size());
        m_generation.push_back(1u);
        m_slot_free.push_back(0u);
    }

    ++m_living;
    return Entity(index, m_generation[index]);
}

//------------------------------------------------------------------------------
void EntityRegistry::destroy(Entity p_entity)
{
    if (!alive(p_entity))
    {
        return;
    }
    const std::uint16_t index = p_entity.index();

    // Bump the generation. Wrap-around: skip zero, which would revive the
    // handles of an already-freed slot.
    std::uint16_t next = static_cast<std::uint16_t>(m_generation[index] + 1u);
    if (next == 0u)
    {
        next = 1u;
    }
    m_generation[index] = next;
    m_slot_free[index] = 1u;
    m_free.push_back(index);
    --m_living;
}

//------------------------------------------------------------------------------
bool EntityRegistry::alive(Entity p_entity) const
{
    if (!p_entity.valid())
    {
        return false;
    }
    const std::uint16_t index = p_entity.index();
    if (index >= m_generation.size())
    {
        return false;
    }
    if (m_slot_free[index] != 0u)
    {
        return false;
    }
    return m_generation[index] == p_entity.generation();
}

} // namespace world
