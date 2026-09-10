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

#include <cstdint>
#include <functional>

namespace world
{

// ****************************************************************************
//! \brief Names one identity inside a World, and knows when that identity is
//! gone.
//!
//! An Entity is a handle: an index into the registry's slot arrays plus a
//! generation counter, packed in 32 bits. Copying is free. Destroying an entity
//! bumps its slot's generation, so a leftover handle is stale rather than
//! silently naming whatever reused the slot.
//!
//! An Entity carries no data of its own. It is not a "GameObject": components
//! sit in their own stores keyed by Entity, and spatial relations live in the
//! SpatialGraph. An Entity may be spatial (owns a SpatialNode) or not.
//!
//! An empty Entity is exactly zero. Live generations start at one, so slot zero
//! is an ordinary first entity and not a missing one.
// ****************************************************************************
class Entity
{
public:

    //! \brief How many entities can be alive at the same time.
    static constexpr std::uint32_t MAX_COUNT = 0xFFFFu;

    // ------------------------------------------------------------------------
    //! \brief An empty entity, naming nothing.
    // ------------------------------------------------------------------------
    constexpr Entity() = default;

    // ------------------------------------------------------------------------
    //! \brief Name the identity living in a slot, at a given generation.
    //!
    //! Only an EntityRegistry has any business calling this.
    //!
    //! \param[in] p_index which slot in the registry.
    //! \param[in] p_generation how many times that slot has been used, starting
    //! at 1. Zero is what makes an entity empty, so it is never a live value.
    // ------------------------------------------------------------------------
    constexpr Entity(std::uint16_t p_index, std::uint16_t p_generation)
        : m_bits((static_cast<std::uint32_t>(p_generation) << 16u) |
                 static_cast<std::uint32_t>(p_index))
    {
    }

    // ------------------------------------------------------------------------
    //! \brief Does this handle name anything at all?
    //!
    //! Says nothing about whether the entity is still alive: only the registry
    //! knows that. This merely tells an empty handle from a filled one.
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr bool valid() const
    {
        return m_bits != 0u;
    }

    [[nodiscard]] constexpr explicit operator bool() const
    {
        return valid();
    }

    [[nodiscard]] constexpr std::uint16_t index() const
    {
        return static_cast<std::uint16_t>(m_bits & 0xFFFFu);
    }

    [[nodiscard]] constexpr std::uint16_t generation() const
    {
        return static_cast<std::uint16_t>(m_bits >> 16u);
    }

    [[nodiscard]] constexpr std::uint32_t bits() const
    {
        return m_bits;
    }

    [[nodiscard]] constexpr bool operator==(Entity const& p_other) const
    {
        return m_bits == p_other.m_bits;
    }

    [[nodiscard]] constexpr bool operator!=(Entity const& p_other) const
    {
        return m_bits != p_other.m_bits;
    }

    [[nodiscard]] constexpr auto operator<=>(Entity const& p_other) const
    {
        return m_bits <=> p_other.m_bits;
    }

private:

    std::uint32_t m_bits = 0u;
};

} // namespace world

template <>
struct std::hash<world::Entity>
{
    [[nodiscard]] std::size_t operator()(world::Entity const& p_entity) const
    {
        return std::hash<std::uint32_t>{}(p_entity.bits());
    }
};
