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

#include "04_World/25_MiscLookAt.hpp"

#include "Assets/Primitives.hpp"
#include "GPU/RenderPass.hpp"
#include "Math/Units.hpp"
#include "Render/Extractor.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/MeshRenderer.hpp"
#include "World/LookAt.hpp"

#include <cmath>
#include <random>
#include <string>

using namespace units::literals;

namespace examples
{

//------------------------------------------------------------------------------
std::string MiscLookAt::description() const
{
    return "Adaptation of three.js misc_lookat.html: one thousand cones "
           "lookAt a sphere moving on a Lissajous path, and the camera eases "
           "toward the mouse while watching the origin. Cones are built with "
           "tip on -Z; the sphere uses the legacy NormalsMaterial.";
}

//------------------------------------------------------------------------------
gpu::Status MiscLookAt::setUp()
{
    GPU_TRY_ASSIGN(cone_asset, assets::makeCone(10.0f, 0.0f, 100.0f, 12u));
    GPU_TRY_ASSIGN(cone_id, m_assets.addMesh("cone", std::move(cone_asset)));
    m_cone_mesh = cone_id;

    GPU_TRY_ASSIGN(sphere_asset, assets::makeSphere(100.0f, 20u, 20u));
    GPU_TRY_ASSIGN(sphere_id,
                   m_assets.addMesh("sphere", std::move(sphere_asset)));
    m_sphere_mesh = sphere_id;

    GPU_TRY_ASSIGN(lit, assets::makeLitMaterial());
    GPU_TRY_ASSIGN(lit_id, m_assets.addMaterial("lit", std::move(lit)));
    m_lit_material = lit_id;

    GPU_TRY_ASSIGN(normals, assets::makeNormalsMaterial());
    GPU_TRY_ASSIGN(normals_id,
                   m_assets.addMaterial("normals", std::move(normals)));
    m_normals_material = normals_id;

    assets::MaterialInstance cone_instance;
    cone_instance.material = m_lit_material;
    cone_instance.color = Vector3f(0.55f, 0.55f, 0.58f);
    GPU_TRY_ASSIGN(cone_mat,
                   m_assets.addMaterialInstance("cone", cone_instance));
    m_cone_material = cone_mat;

    assets::MaterialInstance sphere_instance;
    sphere_instance.material = m_normals_material;
    sphere_instance.opacity = 1.0f;
    GPU_TRY_ASSIGN(sphere_mat,
                   m_assets.addMaterialInstance("sphere", sphere_instance));
    m_sphere_material = sphere_mat;

    std::mt19937 rng(42u);
    std::uniform_real_distribution<float> place(-2000.0f, 2000.0f);
    std::uniform_real_distribution<float> scale(2.0f, 6.0f);

    m_cones.reserve(CONE_COUNT);
    for (std::size_t i = 0u; i < CONE_COUNT; ++i)
    {
        world::Entity entity = m_world.create();
        world::LocalTransform& local = m_world.transform(entity);
        local.position =
            Vector3f(place(rng), place(rng), place(rng));
        const float s = scale(rng);
        local.scale = Vector3f(s, s, s);
        m_world.add(entity,
                    world::MeshRenderer{ m_cone_mesh, m_cone_material });
        m_cones.push_back(entity);
    }

    m_sphere = m_world.create("Target");
    m_world.add(m_sphere,
                world::MeshRenderer{ m_sphere_mesh, m_sphere_material });

    m_camera = m_world.create("Camera");
    world::Camera camera;
    camera.projection = world::Projection::Perspective;
    camera.fov_degrees = 40.0f;
    camera.near_plane = 1.0f;
    camera.far_plane = 15000.0f;
    m_world.add(m_camera, camera);
    m_world.transform(m_camera).position = Vector3f(0.0f, 0.0f, 3200.0f);

    m_scene.setActiveCamera(m_camera);
    m_scene.renderSettings().clear_color = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
    m_scene.environment().ambient = Vector3f(0.45f, 0.45f, 0.45f);
    m_scene.environment().default_light_direction =
        Vector3f(0.3f, -0.8f, -0.5f);

    m_world.update();
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status MiscLookAt::draw(Frame const& p_frame)
{
    const float t = p_frame.total;
    const Vector3f target_pos(
        std::sin(t * 0.7f) * 900.0f - 25.0f,
        std::cos(t * 0.5f) * 400.0f - 25.0f,
        std::cos(t * 0.3f) * 900.0f - 25.0f);
    m_world.transform(m_sphere).position = target_pos;

    for (world::Entity cone : m_cones)
    {
        world::lookAt(m_world.transform(cone), target_pos);
    }

    const float half_w = static_cast<float>(p_frame.width) * 0.5f;
    const float half_h = static_cast<float>(p_frame.height) * 0.5f;
    const float mouse_x = (p_frame.mouse.x - half_w) * 0.25f;
    const float mouse_y = (p_frame.mouse.y - half_h) * 0.25f;

    world::LocalTransform& camera = m_world.transform(m_camera);
    camera.position.x += (mouse_x - camera.position.x) * 0.05f;
    camera.position.y += (mouse_y - camera.position.y) * 0.05f;
    world::lookAt(camera, Vector3f(0.0f, 0.0f, 0.0f));

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
