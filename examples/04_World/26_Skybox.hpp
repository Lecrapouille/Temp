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
#include "GPU/Pipeline.hpp"
#include "GPU/Shader.hpp"
#include "GPU/Texture.hpp"
#include "Render/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "World/Entity.hpp"
#include "World/World.hpp"

namespace examples
{

class Skybox: public Example
{
public:

    Skybox() : m_scene(m_world, m_assets) {}

    [[nodiscard]] std::string name() const override { return "26_Skybox"; }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    [[nodiscard]] gpu::Status drawSkybox(render::RenderSnapshot const& p_camera);

    assets::AssetManager m_assets;
    world::World m_world;
    scene::Scene m_scene;
    render::Renderer m_renderer;

    gpu::Program m_sky_program;
    gpu::Pipeline m_sky_pipeline;
    gpu::Texture m_sky_texture;

    assets::MeshAssetId m_room_mesh;
    assets::MeshAssetId m_cube_mesh;
    assets::MaterialInstanceId m_cube_material;

    world::Entity m_camera;
    world::Entity m_cube;
};

} // namespace examples
