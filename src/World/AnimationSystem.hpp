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

namespace assets
{
class AssetManager;
}

namespace world
{
class World;

// ****************************************************************************
//! \brief Samples AnimationClips onto local transforms, then skins meshes.
//!
//! Call order inside \c tick:
//! 1. write local TRS from every playing Animator;
//! 2. \c World::update so joint world matrices are current;
//! 3. rebuild each skinned MeshAsset vertex buffer from the rest pose.
// ****************************************************************************
class AnimationSystem
{
public:

    [[nodiscard]] static gloop::Status tick(World& p_world,
                                          assets::AssetManager& p_assets,
                                          float p_dt);

    [[nodiscard]] static gloop::Status sample(World& p_world,
                                            assets::AssetManager const& p_assets,
                                            float p_dt);

    [[nodiscard]] static gloop::Status skin(World& p_world,
                                          assets::AssetManager& p_assets);
};

} // namespace world
