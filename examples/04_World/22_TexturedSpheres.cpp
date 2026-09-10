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

#include "04_World/22_TexturedSpheres.hpp"

#include "Assets/Primitives.hpp"
#include "Assets/TextureAsset.hpp"
#include "Common/DataPath.hpp"
#include "GPU/RenderPass.hpp"
#include "GPU/Texture.hpp"
#include "Math/Units.hpp"
#include "Render/Extractor.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/Light.hpp"
#include "World/Components/MeshRenderer.hpp"

#include <cmath>

using namespace units::literals;

namespace examples
{

//------------------------------------------------------------------------------
std::string TexturedSpheres::description() const
{
    return "makeSphere builds a UV sphere with normals and UVs. "
           "makePbrMaterial comes from ShaderLib. A PNG from the data "
           "repository becomes a TextureAsset; MaterialInstances mix "
           "baseColorFactor with optional albedo maps. The camera orbits "
           "slowly so --check sees every sphere.";
}

//------------------------------------------------------------------------------
gpu::Status TexturedSpheres::setUp()
{
    GPU_TRY_ASSIGN(sphere_mesh, assets::makeSphere(0.5f, 24u, 32u));
    GPU_TRY_ASSIGN(mesh_id, m_assets.addMesh("sphere", std::move(sphere_mesh)));
    m_sphere_mesh = mesh_id;

    GPU_TRY_ASSIGN(pbr, assets::makePbrMaterial());
    GPU_TRY_ASSIGN(material_id, m_assets.addMaterial("pbr", std::move(pbr)));
    m_pbr_material = material_id;

    const std::string texture_path = dataPath("grassFlowers.png");
    if (!texture_path.empty())
    {
        gpu::LoadOptions options;
        options.srgb = true;
        GPU_TRY_ASSIGN(gpu_texture,
                       gpu::Texture::fromFile(texture_path, options));
        assets::TextureAsset texture;
        texture.name = "grassFlowers";
        texture.texture = std::move(gpu_texture);
        GPU_TRY_ASSIGN(texture_id,
                       m_assets.addTexture(texture.name, std::move(texture)));
        m_grass_texture = texture_id;
    }

    GPU_TRY_ASSIGN(
        red_id,
        m_assets.addMaterialInstance(
            "red",
            assets::MaterialInstance{
                m_pbr_material,
                Vector3f(1.0f, 1.0f, 1.0f),
                Vector3f(0.85f, 0.25f, 0.20f),
                {} }));
    m_instances[0] = red_id;

    GPU_TRY_ASSIGN(
        green_id,
        m_assets.addMaterialInstance(
            "textured",
            assets::MaterialInstance{
                m_pbr_material,
                Vector3f(1.0f, 1.0f, 1.0f),
                Vector3f(1.0f, 1.0f, 1.0f),
                m_grass_texture }));
    m_instances[1] = green_id;

    GPU_TRY_ASSIGN(
        blue_id,
        m_assets.addMaterialInstance(
            "blue",
            assets::MaterialInstance{
                m_pbr_material,
                Vector3f(1.0f, 1.0f, 1.0f),
                Vector3f(0.25f, 0.45f, 0.90f),
                {} }));
    m_instances[2] = blue_id;

    world::Entity root = m_world.create("Root");
    const std::array<Vector3f, 3u> places{
        Vector3f(-2.5f, 0.5f, 0.0f),
        Vector3f(0.0f, 0.5f, 0.0f),
        Vector3f(2.5f, 0.5f, 0.0f)
    };
    for (std::size_t i = 0u; i < places.size(); ++i)
    {
        world::Entity entity = m_world.create();
        m_world.transform(entity).position = places[i];
        m_world.transform(entity).scale = Vector3f(1.8f);
        m_world.add(entity,
                    world::MeshRenderer{ m_sphere_mesh, m_instances[i] });
        GPU_TRY(m_world.setParent(entity, root));
        m_spheres.push_back(entity);
    }

    m_sun = m_world.create("Sun");
    m_world.transform(m_sun).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(-0.7),
                             Vector3f(1.0f, 0.0f, 0.0f));
    m_world.add(m_sun,
                world::DirectionalLight{ Vector3f(1.0f, 0.96f, 0.90f), 1.0f });

    m_camera = m_world.create("Camera");
    world::Camera camera;
    camera.projection = world::Projection::Perspective;
    camera.fov_degrees = 50.0f;
    camera.near_plane = 0.1f;
    camera.far_plane = 200.0f;
    m_world.add(m_camera, camera);
    m_world.transform(m_camera).position = Vector3f(0.0f, 2.5f, 9.0f);

    m_scene.setActiveCamera(m_camera);
    m_scene.renderSettings().clear_color = Vector4f(0.08f, 0.10f, 0.14f, 1.0f);
    m_scene.environment().ambient = Vector3f(0.16f, 0.16f, 0.18f);

    m_world.update();
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status TexturedSpheres::draw(Frame const& p_frame)
{
    const float t = p_frame.total * 0.35f;
    const float radius = 9.0f;
    m_world.transform(m_camera).position =
        Vector3f(radius * std::sin(t), 2.5f, radius * std::cos(t));
    const Quatf yaw = Quatf::fromAngleAxis(units::angle::radian_t(t),
                                           Vector3f(0.0f, 1.0f, 0.0f));
    const Quatf pitch = Quatf::fromAngleAxis(
        units::angle::radian_t(-0.25), Vector3f(1.0f, 0.0f, 0.0f));
    m_world.transform(m_camera).rotation = yaw * pitch;

    for (std::size_t i = 0u; i < m_spheres.size(); ++i)
    {
        m_world.transform(m_spheres[i]).rotation = Quatf::fromAngleAxis(
            units::angle::radian_t(
                p_frame.total * (0.4f + (0.1f * static_cast<float>(i)))),
            Vector3f(0.0f, 1.0f, 0.0f));
    }

    m_world.update();
    GPU_TRY_ASSIGN(
        snapshot,
        render::Extractor::extract(m_scene, p_frame.width, p_frame.height));

    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = m_scene.renderSettings().clear_color;
    desc.clear_depth = true;
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
    return m_renderer.render(pass, snapshot, m_assets);
}

} // namespace examples
