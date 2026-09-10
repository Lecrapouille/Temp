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

#include "World/Components/Camera.hpp"
#include "World/Components/Light.hpp"
#include "World/Components/MeshRenderer.hpp"
#include "World/TransformStore.hpp"

#include <optional>
#include <string>
#include <vector>

namespace assets
{

// ****************************************************************************
//! \brief A MeshRenderer inside a prefab, referencing assets by registered
//! name rather than by runtime id.
// ****************************************************************************
struct PrefabMeshRenderer
{
    std::string mesh;
    std::string material_instance;
    world::RenderFlags flags = world::RenderFlags::None;
};

// ****************************************************************************
//! \brief One node of a prefab tree: transform, optional components, children.
// ****************************************************************************
struct PrefabNode
{
    std::string name;
    world::LocalTransform transform{};
    bool enabled = true;
    std::optional<PrefabMeshRenderer> mesh_renderer;
    std::optional<world::Camera> camera;
    std::optional<world::DirectionalLight> directional_light;
    std::vector<PrefabNode> children;
};

// ****************************************************************************
//! \brief A reusable entity hierarchy template owned by the AssetManager.
//!
//! Asset references inside the tree are strings (``"cube"``, ``"wood"``) so a
//! prefab survives save/load and can be shared across Worlds. Runtime ids are
//! resolved only at instantiation time.
// ****************************************************************************
struct Prefab
{
    std::string name;
    PrefabNode root;
};

} // namespace assets
