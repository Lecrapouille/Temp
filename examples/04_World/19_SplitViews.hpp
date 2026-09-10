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

#include <optional>
#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief One World, two Scenes, two passes side by side in the same window.
//!
//! The demo builds a small procedural landscape (a ring of towers around a
//! turning pillar) once, then renders it twice per frame:
//! - on the left, a perspective camera orbiting at ground level;
//! - on the right, an orthographic camera looking straight down, the way a
//!   minimap does.
//!
//! What matters is what is *not* duplicated. The World, the AssetManager and
//! the shared meshes/materials all exist once. Each Scene is a pointer at
//! that World plus an Entity for its camera, so two views cost two Scenes
//! and two RenderPasses, not two Worlds. Extraction runs twice (each view
//! has its own frustum, so different objects survive), and the Renderer's
//! queue is filled twice, but the simulation itself moves once per frame.
//!
//! Adding a third view (a debug camera on the right side, an off-screen
//! capture, a rear-view mirror) is one more Scene and one more pass. The
//! architecture pays for that shape.
// ****************************************************************************
class SplitViews: public Example
{
public:

    SplitViews();

    [[nodiscard]] std::string name() const override { return "19_SplitViews"; }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    //! \brief One shared World, one shared AssetManager.
    assets::AssetManager m_assets;
    world::World m_world;

    //! \brief Two Scenes into that World. Each holds its own camera entity
    //! and its own render settings.
    std::optional<scene::Scene> m_scene_perspective;
    std::optional<scene::Scene> m_scene_top;

    render::Renderer m_renderer;

    assets::MeshAssetId m_cube_mesh;
    assets::MaterialId m_lit_material;
    assets::MaterialInstanceId m_pillar_color;
    assets::MaterialInstanceId m_tower_color;
    assets::MaterialInstanceId m_ground_color;

    world::Entity m_camera_perspective;
    world::Entity m_camera_top;
    world::Entity m_sun;
    world::Entity m_root;
    world::Entity m_pillar;
    std::vector<world::Entity> m_towers;
};

} // namespace examples
