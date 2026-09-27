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

namespace scene
{

// ****************************************************************************
//! \brief Marks the root of an entity subtree spawned from a prefab template.
//!
//! The component is data only: it records which prefab was used. Gameplay
//! systems can use it to reset an instance or to find animation roots.
// ****************************************************************************
struct PrefabInstance
{
    scene::PrefabId prefab{};
};

} // namespace scene
