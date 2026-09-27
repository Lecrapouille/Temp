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

#include <cstdint>
#include <functional>

namespace scene
{

// ****************************************************************************
//! \brief A stable identifier for an asset held by the AssetManager.
//!
//! The tag makes the type of asset part of the type of the identifier, so a
//! MaterialInstanceId cannot be passed where a MeshAssetId is expected. The tag
//! is never defined, only named:
//! \code
//! using MeshAssetId = scene::AssetId<struct MeshTag>;
//! \endcode
//!
//! An empty id is exactly zero. It is not a live asset; live generations start
//! at one.
//!
//! An id is stable from the point of view of the engine: recreating the GPU
//! ressources of an asset after a context loss does not change the id, so
//! World components holding an id do not need to be rewritten.
// ****************************************************************************
template <typename Tag>
class AssetId
{
public:

    constexpr AssetId() = default;

    constexpr AssetId(std::uint16_t p_index, std::uint16_t p_generation)
        : m_bits((static_cast<std::uint32_t>(p_generation) << 16u) |
                 static_cast<std::uint32_t>(p_index))
    {
    }

    [[nodiscard]] constexpr bool valid() const { return m_bits != 0u; }
    [[nodiscard]] constexpr explicit operator bool() const { return valid(); }
    [[nodiscard]] constexpr std::uint16_t index() const
    {
        return static_cast<std::uint16_t>(m_bits & 0xFFFFu);
    }
    [[nodiscard]] constexpr std::uint16_t generation() const
    {
        return static_cast<std::uint16_t>(m_bits >> 16u);
    }
    [[nodiscard]] constexpr std::uint32_t bits() const { return m_bits; }

    [[nodiscard]] constexpr bool operator==(AssetId const& p_other) const
    {
        return m_bits == p_other.m_bits;
    }
    [[nodiscard]] constexpr bool operator!=(AssetId const& p_other) const
    {
        return m_bits != p_other.m_bits;
    }
    [[nodiscard]] constexpr auto operator<=>(AssetId const& p_other) const
    {
        return m_bits <=> p_other.m_bits;
    }

private:

    std::uint32_t m_bits = 0u;
};

//! \brief Names one mesh in the AssetManager.
using MeshAssetId = AssetId<struct MeshAssetTag>;
//! \brief Names one texture in the AssetManager.
using TextureAssetId = AssetId<struct TextureAssetTag>;
//! \brief Names one material family in the AssetManager.
using MaterialId = AssetId<struct MaterialTag>;
//! \brief Names one instance of a material in the AssetManager.
using MaterialInstanceId = AssetId<struct MaterialInstanceTag>;
//! \brief Names one prefab template in the AssetManager.
using PrefabId = AssetId<struct PrefabTag>;
//! \brief Names one sampled animation clip in the AssetManager.
using AnimationClipId = AssetId<struct AnimationClipTag>;
//! \brief Names one inverse-bind skeleton in the AssetManager.
using SkinAssetId = AssetId<struct SkinAssetTag>;

} // namespace scene

template <typename Tag>
struct std::hash<scene::AssetId<Tag>>
{
    [[nodiscard]] std::size_t operator()(scene::AssetId<Tag> const& p_id) const
    {
        return std::hash<std::uint32_t>{}(p_id.bits());
    }
};
