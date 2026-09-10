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

#include "04_World/20_CameraPick.hpp"

#include "Assets/Primitives.hpp"
#include "GPU/RenderPass.hpp"
#include "Math/Units.hpp"
#include "Render/Extractor.hpp"
#include "Render/Picker.hpp"
#include "World/CameraInput.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/Light.hpp"
#include "World/Components/MeshRenderer.hpp"

using namespace units::literals;

namespace examples
{

namespace
{

world::CameraInput inputFrom(Frame const& p_frame)
{
    world::CameraInput input;
    input.look_delta = p_frame.mouse_delta;
    input.zoom_delta = p_frame.scroll;
    input.look = p_frame.mouse_right;
    input.forward = p_frame.key_w;
    input.back = p_frame.key_s;
    input.left = p_frame.key_a;
    input.right = p_frame.key_d;
    input.up = p_frame.key_q;
    input.down = p_frame.key_e;
    input.boost = p_frame.key_shift;
    return input;
}

} // namespace

//------------------------------------------------------------------------------
std::string CameraPick::description() const
{
    return "A camera is an Entity; a controller writes its transform. "
           "Orbit (1), fly (2) or walk (3), drag with the right mouse, "
           "scroll to zoom, WASD/QE to move. A left click unprojects the "
           "pixel into a world ray and picks the closest MeshRenderer AABB. "
           "Selection is a MaterialInstance swap: the World never draws.";
}

//------------------------------------------------------------------------------
gpu::Status CameraPick::setUp()
{
    GPU_TRY_ASSIGN(cube_asset, assets::makeCube());
    GPU_TRY_ASSIGN(mesh_id, m_assets.addMesh("cube", std::move(cube_asset)));
    m_cube_mesh = mesh_id;

    GPU_TRY_ASSIGN(lit, assets::makeLitMaterial());
    GPU_TRY_ASSIGN(material_id, m_assets.addMaterial("lit", std::move(lit)));
    m_lit_material = material_id;

    GPU_TRY_ASSIGN(
        plain_id,
        m_assets.addMaterialInstance(
            "plain",
            assets::MaterialInstance{ m_lit_material,
                                      Vector3f(0.55f, 0.62f, 0.78f) }));
    m_plain = plain_id;
    GPU_TRY_ASSIGN(
        picked_id,
        m_assets.addMaterialInstance(
            "picked",
            assets::MaterialInstance{ m_lit_material,
                                      Vector3f(0.95f, 0.72f, 0.22f) }));
    m_picked = picked_id;

    world::Entity root = m_world.create("Root");
    world::Entity ground = m_world.create("Ground");
    m_world.transform(ground).position = Vector3f(0.0f, -0.5f, 0.0f);
    m_world.transform(ground).scale = Vector3f(40.0f, 1.0f, 40.0f);
    m_world.add(ground, world::MeshRenderer{ m_cube_mesh, m_plain });
    GPU_TRY(m_world.setParent(ground, root));

    // A 3x3 of cubes sitting on the ground, spaced so a click cannot
    // ambiguously hit two AABBs at the same t.
    for (int x = -1; x <= 1; ++x)
    {
        for (int z = -1; z <= 1; ++z)
        {
            world::Entity cube = m_world.create();
            m_world.transform(cube).position =
                Vector3f(static_cast<float>(x) * 4.0f,
                         1.0f,
                         static_cast<float>(z) * 4.0f);
            m_world.transform(cube).scale = Vector3f(2.0f);
            m_world.add(cube, world::MeshRenderer{ m_cube_mesh, m_plain });
            GPU_TRY(m_world.setParent(cube, root));
            m_cubes.push_back(cube);
        }
    }

    m_sun = m_world.create("Sun");
    m_world.transform(m_sun).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(-0.9),
                             Vector3f(1.0f, 0.0f, 0.0f));
    m_world.add(m_sun,
                world::DirectionalLight{ Vector3f(1.0f, 0.96f, 0.88f), 1.0f });

    m_camera = m_world.create("Camera");
    world::Camera camera;
    camera.projection = world::Projection::Perspective;
    camera.fov_degrees = 55.0f;
    camera.near_plane = 0.2f;
    camera.far_plane = 400.0f;
    m_world.add(m_camera, camera);

    m_orbit.target = Vector3f(0.0f, 1.0f, 0.0f);
    m_orbit.distance = 22.0f;
    m_orbit.pitch = -0.35f;
    m_fly.yaw = 0.0f;
    m_fly.pitch = -0.2f;
    m_fps.eye_height = 3.0f;
    m_fps.yaw = 0.0f;
    m_fps.pitch = -0.15f;
    m_world.transform(m_camera).position = Vector3f(0.0f, 3.0f, 22.0f);

    m_scene.setActiveCamera(m_camera);
    m_scene.renderSettings().clear_color = Vector4f(0.06f, 0.08f, 0.14f, 1.0f);
    m_scene.environment().ambient = Vector3f(0.14f, 0.15f, 0.18f);

    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status CameraPick::draw(Frame const& p_frame)
{
    if (p_frame.key_1)
    {
        m_mode = Mode::Orbit;
    }
    else if (p_frame.key_2)
    {
        m_mode = Mode::Fly;
    }
    else if (p_frame.key_3)
    {
        m_mode = Mode::FPS;
    }

    const world::CameraInput input = inputFrom(p_frame);
    if (m_mode == Mode::Orbit)
    {
        // A slow self-orbit so --check records a readable angle, plus any
        // right-drag the user adds.
        m_orbit.yaw = p_frame.total * 0.35f;
        m_orbit.apply(m_world, m_camera, input);
    }
    else if (m_mode == Mode::Fly)
    {
        m_fly.apply(m_world, m_camera, input, p_frame.elapsed);
    }
    else
    {
        m_fps.apply(m_world, m_camera, input, p_frame.elapsed);
    }

    m_world.update();
    GPU_TRY_ASSIGN(
        snapshot,
        render::Extractor::extract(m_scene, p_frame.width, p_frame.height));

    const bool auto_pick = !m_auto_picked && (p_frame.total > 0.05f);
    if (p_frame.mouse_left_pressed || auto_pick)
    {
        const float px = p_frame.mouse_left_pressed
                             ? p_frame.mouse.x
                             : (static_cast<float>(p_frame.width) * 0.5f);
        const float py = p_frame.mouse_left_pressed
                             ? p_frame.mouse.y
                             : (static_cast<float>(p_frame.height) * 0.45f);
        auto hit = render::pickAt(m_scene,
                                  snapshot.camera,
                                  px,
                                  py,
                                  p_frame.width,
                                  p_frame.height);
        if (m_selection.valid() && m_world.alive(m_selection) &&
            m_world.has<world::MeshRenderer>(m_selection))
        {
            m_world.get<world::MeshRenderer>(m_selection).material_instance =
                m_plain;
        }
        m_selection = {};
        if (hit.has_value() && (m_world.name(hit->entity) != "Ground"))
        {
            m_selection = hit->entity;
            m_world.get<world::MeshRenderer>(m_selection).material_instance =
                m_picked;
        }
        m_auto_picked = true;
        GPU_TRY_ASSIGN(
            refreshed,
            render::Extractor::extract(m_scene, p_frame.width, p_frame.height));
        snapshot = std::move(refreshed);
    }

    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = m_scene.renderSettings().clear_color;
    desc.clear_depth = true;
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
    return m_renderer.render(pass, snapshot, m_assets);
}

} // namespace examples
