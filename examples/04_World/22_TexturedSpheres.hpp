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
#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief UV spheres built in code, lit with the PBR material path.
//!
//! Three ideas that 21_GltfModel does not isolate:
//! - \c makeSphere() as a mesh primitive (like Three.js SphereGeometry);
//! - \c TextureAsset registered in the AssetManager and referenced from a
//!   MaterialInstance;
//! - \c makePbrMaterial() from ShaderLib, with per-instance base colour
//!   factors and optional albedo maps.
// ****************************************************************************
class TexturedSpheres: public Example
{
public:

    TexturedSpheres() : m_scene(m_world, m_assets) {}

    [[nodiscard]] std::string name() const override
    {
        return "22_TexturedSpheres";
    }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    assets::AssetManager m_assets;
    world::World m_world;
    scene::Scene m_scene;
    render::Renderer m_renderer;

    assets::MeshAssetId m_sphere_mesh;
    assets::MaterialId m_pbr_material;
    assets::TextureAssetId m_grass_texture;
    std::array<assets::MaterialInstanceId, 3u> m_instances{};
    world::Entity m_camera;
    world::Entity m_sun;
    std::vector<world::Entity> m_spheres;
};

} // namespace examples
