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
#include "World/Entity.hpp"
#include "World/World.hpp"

#include <array>

namespace examples
{

// ****************************************************************************
//! \brief A walk-cycle character built as a hierarchy of joints.
//!
//! 17_MovingRobot waves its arms in place. This one is a model: the root
//! walks a circle, hips and shoulders swing in opposite phase, and the
//! visible parts (cylinders, a sphere, boxes) hang under unit-scale joints
//! so a rotation never squashes a limb. The pose is rewritten each frame
//! from \c sin(total), the same deterministic pattern as the robot.
// ****************************************************************************
class AnimatedModel: public Example
{
public:

    AnimatedModel() : m_scene(m_world, m_assets) {}

    [[nodiscard]] std::string name() const override
    {
        return "34_AnimatedModel";
    }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    struct Walker
    {
        world::Entity root;
        world::Entity torso;
        world::Entity head;
        world::Entity left_shoulder;
        world::Entity right_shoulder;
        world::Entity left_hip;
        world::Entity right_hip;
        float phase = 0.0f;
        float radius = 3.2f;
    };

    [[nodiscard]] gpu::Status makeWalker(Walker& p_walker,
                                         char const* p_name,
                                         float p_phase,
                                         float p_radius,
                                         assets::MaterialInstanceId p_shirt,
                                         assets::MaterialInstanceId p_limb);
    void poseWalker(Walker& p_walker, float p_time);

    assets::AssetManager m_assets;
    world::World m_world;
    scene::Scene m_scene;
    render::Renderer m_renderer;

    assets::MeshAssetId m_cube_mesh;
    assets::MeshAssetId m_sphere_mesh;
    assets::MeshAssetId m_limb_mesh;
    assets::MaterialInstanceId m_ground_mat;
    assets::MaterialInstanceId m_shirt_a;
    assets::MaterialInstanceId m_shirt_b;
    assets::MaterialInstanceId m_limb_mat;
    assets::MaterialInstanceId m_skin_mat;

    world::Entity m_camera;
    world::Entity m_sun;
    std::array<Walker, 2u> m_walkers{};
};

} // namespace examples
