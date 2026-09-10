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
//! \brief Three robots of cubes, animated by rewriting their local poses from
//! the frame's total time.
//!
//! The old scene-graph demo hung a body, a head, two arms and two legs under
//! each robot, then rotated the locals and let the world matrices follow. The
//! interesting part is unchanged: the head does not know it is on a turning
//! body. What is different is the animation loop: each frame the robot's
//! joints are written from \c sin/cos of \c Frame::total, not from an
//! accumulated per-frame delta. That makes the motion deterministic (bras and
//! head never drift out of the frustum after a few thousand frames), and shows
//! the intended pattern: authoring code writes the local transforms; the
//! World, the Extractor and the Renderer do not care what wrote them.
//!
//! Compared to the older prototype this exercises the whole stack of the
//! target architecture:
//! - the cube and the lit material are registered in an AssetManager;
//! - each part is a MeshRenderer holding an AssetId, not a pointer;
//! - the parts of a robot are a hierarchy of unit joints, each carrying its
//!   own scale, so scale propagation follows the standard TRS composition
//!   without twisting the shape of a child;
//! - the camera is an Entity of the World with a Camera component; the Scene
//!   picks it as its active view;
//! - the frame is: World::update() then Extractor::extract() then
//!   Renderer::render(). The World never touches gpu::.
// ****************************************************************************
class MovingRobot: public Example
{
public:

    MovingRobot() : m_scene(m_world, m_assets) {}

    [[nodiscard]] std::string name() const override
    {
        return "17_MovingRobot";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    // ------------------------------------------------------------------------
    //! \brief One robot, kept as the entities the animation pose every frame.
    //!
    //! Only the joints (\c root, \c body, \c head, \c left_shoulder,
    //! \c right_shoulder) are rotated. Their mesh children carry the scale
    //! and never move relative to their joint, which is why a swinging arm
    //! keeps its shape.
    // ------------------------------------------------------------------------
    struct Robot
    {
        world::Entity root;
        world::Entity body;
        world::Entity head;
        world::Entity left_shoulder;
        world::Entity right_shoulder;
        //! \brief Phase offset in seconds. Each robot animates from the same
        //! clock, offset so the three swings do not look like one motion
        //! copied three times.
        float phase = 0.0f;
    };

    [[nodiscard]] gpu::Status makeRobot(Robot& p_robot,
                                        char const* p_name,
                                        Vector3f const& p_place,
                                        float p_phase);

    // The stack, in the order the architecture puts it:
    assets::AssetManager m_assets;
    world::World m_world;
    scene::Scene m_scene;
    render::Renderer m_renderer;

    // Handles into the AssetManager for what this demo needs.
    assets::MeshAssetId m_cube_mesh;
    assets::MaterialId m_lit_material;
    assets::MaterialInstanceId m_wood;
    assets::MaterialInstanceId m_dark;
    assets::MaterialInstanceId m_light;

    world::Entity m_camera;
    world::Entity m_scene_root;
    world::Entity m_sun;
    std::array<Robot, 3u> m_robots{};
};

} // namespace examples
