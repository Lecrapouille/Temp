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

#include "Compages/Scene/Assets/AssetIds.hpp"
#include "Compages/Core/Result.hpp"
#include "Compages/Scene/EntityId.hpp"
#include "Compages/Scene/TransformStore.hpp"

namespace scene
{
class AssetManager;
struct PrefabNode;
} // namespace scene

namespace scene
{
class World;
}

namespace scene
{

// ****************************************************************************
//! \brief Spawn a prefab hierarchy into a World.
//!
//! \param[in,out] p_world where entities are created.
//! \param[in] p_assets used to resolve mesh and material names.
//! \param[in] p_prefab which template to spawn.
//! \param[in] p_parent optional parent entity. Empty means a new root.
//! \param[in] p_root_offset applied to the prefab root transform after copy.
//! \return the root entity of the instance, tagged with PrefabInstance.
// ****************************************************************************
[[nodiscard]] compages::Result<EntityId>
instantiate(World& p_world,
            scene::AssetManager& p_assets,
            scene::PrefabId p_prefab,
            EntityId p_parent = {},
            LocalTransform p_root_offset = {});

} // namespace scene
