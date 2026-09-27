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

namespace scene
{
class AssetManager;
}

namespace scene
{
class World;

// ****************************************************************************
//! \brief Samples AnimationClips onto local transforms, then poses skins.
//!
//! Call order inside \c tick:
//! 1. write local TRS from every playing Animator;
//! 2. \c World::update so joint world matrices are current;
//! 3. \c pose writes ibm * jointWorld * inv(meshWorld) onto each
//!    \c SkinInstance. The rest-pose vertex buffer stays on the GPU;
//!    the vertex shader is what deforms it.
//!
//! \c skin is the CPU prototype: it still rebuilds the VBO. Do not call
//! it from the frame loop.
// ****************************************************************************
class AnimationSystem
{
public:

    [[nodiscard]] static compages::Status tick(World& p_world,
                                          scene::AssetManager& p_assets,
                                          float p_dt);

    [[nodiscard]] static compages::Status sample(World& p_world,
                                            scene::AssetManager const& p_assets,
                                            float p_dt);

    [[nodiscard]] static compages::Status pose(World& p_world,
                                          scene::AssetManager const& p_assets);

    //! \brief CPU vertex deformation. Prototype only.
    [[nodiscard]] static compages::Status skin(World& p_world,
                                          scene::AssetManager& p_assets);
};

} // namespace scene
