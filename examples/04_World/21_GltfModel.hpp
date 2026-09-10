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

#include "Assets/AssetManager.hpp"
#include "Render/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "World/Controllers/OrbitController.hpp"
#include "World/Entity.hpp"
#include "World/World.hpp"

namespace examples
{

// ****************************************************************************
//! \brief Load a static GLB and render it with the PBR material path.
//!
//! \c assets::importGltf turns a file into MeshAssets, TextureAssets,
//! MaterialInstances and a World hierarchy. The loader never calls OpenGL;
//! the AssetManager owns the device resources. The Renderer binds the albedo
//! map when a MaterialInstance carries a TextureAssetId.
// ****************************************************************************
class GltfModel: public Example
{
public:

    GltfModel() : m_scene(m_world, m_assets) {}

    [[nodiscard]] std::string name() const override { return "21_GltfModel"; }
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
    world::Entity m_model_root{};
    world::OrbitController m_orbit;
};

} // namespace examples
