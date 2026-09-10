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

#include "Common/Example.hpp"

#include "Assets/AssetIds.hpp"
#include "Assets/AssetManager.hpp"
#include "Render/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "World/Controllers/OrbitController.hpp"
#include "World/Entity.hpp"
#include "World/World.hpp"

#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Play a skinned glTF clip: Soldier.glb Idle / Walk / Run.
//!
//! \c importGltf now also builds AnimationClips, SkinAssets and an Animator.
//! \c AnimationSystem samples the clip onto joint transforms, then rebuilds
//! the mesh from the rest pose. Keys 1 / 2 / 3 switch Idle / Walk / Run.
// ****************************************************************************
class GltfAnimation: public Example
{
public:

    GltfAnimation() : m_scene(m_world, m_assets) {}

    [[nodiscard]] std::string name() const override
    {
        return "36_GltfAnimation";
    }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    assets::AssetManager m_assets;
    world::World m_world;
    scene::Scene m_scene;
    render::Renderer m_renderer;

    world::Entity m_camera;
    world::Entity m_sun;
    world::Entity m_animator{};
    world::OrbitController m_orbit;
    std::vector<assets::AnimationClipId> m_clips;
};

} // namespace examples
