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

#include "Compages/Core/Result.hpp"
#include "Compages/Scene/EntityId.hpp"

#include <string>
#include <vector>

namespace scene
{
class AssetManager;
}

namespace scene
{
class World;
}

namespace scene
{
class Scene;
}

namespace scene
{

// ****************************************************************************
//! \brief Save and reload a World subtree as JSON.
//!
//! Asset references inside components are stored by their registered name in
//! the AssetManager, not by runtime id, so a scene file survives restarts as
//! long as the same assets are registered before loading.
// ****************************************************************************

// ------------------------------------------------------------------------
//! \brief Write every spatial root of \c p_world to a JSON file.
// ------------------------------------------------------------------------
[[nodiscard]] compages::Status save(scene::World const& p_world,
                               scene::AssetManager const& p_assets,
                               std::string const& p_path);

// ------------------------------------------------------------------------
//! \brief Load entities from a JSON file into \c p_world.
//!
//! \return the entities that became new spatial roots (no parent in the file).
// ------------------------------------------------------------------------
[[nodiscard]] compages::Result<std::vector<scene::EntityId>>
load(scene::World& p_world,
     scene::AssetManager const& p_assets,
     std::string const& p_path);

// ------------------------------------------------------------------------
//! \brief Save a Scene's World plus presentation settings.
// ------------------------------------------------------------------------
[[nodiscard]] compages::Status saveScene(scene::Scene const& p_scene,
                                  std::string const& p_path);

// ------------------------------------------------------------------------
//! \brief Reload a Scene's World and presentation settings.
// ------------------------------------------------------------------------
[[nodiscard]] compages::Result<std::vector<scene::EntityId>>
loadScene(scene::Scene& p_scene, std::string const& p_path);

} // namespace scene
