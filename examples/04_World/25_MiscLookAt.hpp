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

#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Port of three.js \c misc_lookat.html: a moving sphere and a field of
//! cones that track it, while the camera eases toward the mouse.
// ****************************************************************************
class MiscLookAt: public Example
{
public:

    MiscLookAt() : m_scene(m_world, m_assets) {}

    [[nodiscard]] std::string name() const override { return "25_MiscLookAt"; }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    static constexpr std::size_t CONE_COUNT = 1000u;

    assets::AssetManager m_assets;
    world::World m_world;
    scene::Scene m_scene;
    render::Renderer m_renderer;

    assets::MeshAssetId m_cone_mesh;
    assets::MaterialId m_lit_material;
    assets::MaterialId m_normals_material;
    assets::MaterialInstanceId m_cone_material;
    assets::MeshAssetId m_sphere_mesh;
    assets::MaterialInstanceId m_sphere_material;

    world::Entity m_camera;
    world::Entity m_sphere;
    std::vector<world::Entity> m_cones;
};

} // namespace examples
