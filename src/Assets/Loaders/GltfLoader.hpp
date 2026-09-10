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

#include "Assets/AssetIds.hpp"
#include "Assets/AssetManager.hpp"
#include "Common/Result.hpp"
#include "World/Entity.hpp"
#include "World/World.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace assets
{

// ****************************************************************************
//! \brief What \c importGltf hands back after a scene was built.
// ****************************************************************************
struct GltfImport
{
    //! \brief Root of the imported scene graph inside the World.
    world::Entity root{};
    //! \brief Entity that received the Animator when the file has clips.
    world::Entity animator{};
    std::vector<AnimationClipId> animations;
    std::size_t mesh_count = 0u;
    std::size_t texture_count = 0u;
    std::size_t node_count = 0u;
    std::size_t skin_count = 0u;
};

// ****************************************************************************
//! \brief Load a GLB/glTF file into assets and the World hierarchy.
//!
//! The loader never calls OpenGL itself: it builds CPU-side descriptions and
//! asks the AssetManager to create \c gpu:: resources. Cameras are ignored.
//! Skins and animations become SkinAssets, AnimationClips, SkinInstance and
//! Animator components.
//!
//! \param[in] p_path file on disk.
//! \param[in,out] p_assets where meshes, textures and material instances go.
//! \param[in,out] p_world where entities and MeshRenderer components are
//! created.
//! \param[in] p_parent optional parent entity. Empty means a new root.
//! \param[in] p_shared_material optional PBR material family. When empty, a
//! \c makePbrMaterial() is registered once and reused.
// ****************************************************************************
[[nodiscard]] gloop::Result<GltfImport>
importGltf(std::string const& p_path,
           AssetManager& p_assets,
           world::World& p_world,
           world::Entity p_parent = {},
           MaterialId p_shared_material = {});

} // namespace assets
