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
#include "World/Controllers/FPSController.hpp"
#include "World/Controllers/FlyController.hpp"
#include "World/Controllers/OrbitController.hpp"
#include "World/Entity.hpp"
#include "World/World.hpp"

#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Navigate a camera and pick a cube by shooting a ray through a pixel.
//!
//! Three controllers write the same Camera entity:
//! - 1: orbit around the cluster (the default, also what --check records);
//! - 2: fly, six degrees of freedom;
//! - 3: walk on the ground, FPS-style.
//!
//! A left click unprojects the pixel through \c CameraFrame::screenRay and
//! \c render::pick, then swaps the hit MeshRenderer onto a highlight
//! material. The World is not asked to draw a selection outline: selection
//! is data, the renderer just sees a different MaterialInstanceId.
// ****************************************************************************
class CameraPick: public Example
{
public:

    CameraPick() : m_scene(m_world, m_assets) {}

    [[nodiscard]] std::string name() const override { return "20_CameraPick"; }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    enum class Mode
    {
        Orbit,
        Fly,
        FPS
    };

    assets::AssetManager m_assets;
    world::World m_world;
    scene::Scene m_scene;
    render::Renderer m_renderer;

    assets::MeshAssetId m_cube_mesh;
    assets::MaterialId m_lit_material;
    assets::MaterialInstanceId m_plain;
    assets::MaterialInstanceId m_picked;

    world::Entity m_camera;
    world::Entity m_sun;
    std::vector<world::Entity> m_cubes;
    world::Entity m_selection{};
    bool m_auto_picked = false;

    world::OrbitController m_orbit;
    world::FlyController m_fly;
    world::FPSController m_fps;
    Mode m_mode = Mode::Orbit;
};

} // namespace examples
