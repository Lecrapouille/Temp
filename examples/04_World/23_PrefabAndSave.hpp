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
#include <string>

namespace examples
{

// ****************************************************************************
//! \brief Three robot prefabs, animated and serializable to JSON.
//!
//! The robot hierarchy is authored once as a \c Prefab asset, then
//! \c world::instantiate() spawns it three times. \c scene::save() writes
//! the World to disk with asset names instead of runtime ids; the same file
//! can be loaded back after the assets are registered again.
// ****************************************************************************
class PrefabAndSave: public Example
{
public:

    PrefabAndSave() : m_scene(m_world, m_assets) {}

    [[nodiscard]] std::string name() const override
    {
        return "23_PrefabAndSave";
    }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    struct RobotHandles
    {
        world::Entity root;
        world::Entity left_shoulder;
        world::Entity right_shoulder;
        world::Entity head;
        float phase = 0.0f;
    };

    assets::AssetManager m_assets;
    world::World m_world;
    scene::Scene m_scene;
    render::Renderer m_renderer;

    assets::PrefabId m_robot_prefab;
    world::Entity m_camera;
    world::Entity m_sun;
    std::array<RobotHandles, 3u> m_robots{};
    std::string m_scene_path;
    bool m_saved = false;
};

} // namespace examples
