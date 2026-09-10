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

#include "04_World/24_MvpDemo.hpp"

#include "Assets/Primitives.hpp"
#include "Common/InputBridge.hpp"
#include "GPU/RenderPass.hpp"
#include "Math/Units.hpp"
#include "Render/Extractor.hpp"
#include "Scene/SceneSerializer.hpp"
#include "World/Behavior.hpp"
#include "World/BehaviorSystem.hpp"
#include "World/Components/BoxCollider.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/Light.hpp"
#include "World/Components/MeshRenderer.hpp"
#include "World/Components/RigidBody.hpp"
#include "World/Controllers/OrbitController.hpp"

using namespace units::literals;

namespace examples
{

namespace
{

void spinBehavior(world::World& p_world,
                  world::Entity p_entity,
                  float /*p_dt*/,
                  world::InputState const& /*p_input*/,
                  void* /*p_user*/)
{
    p_world.transform(p_entity).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(0.015f),
                             Vector3f(0.0f, 1.0f, 0.0f)) *
        p_world.transform(p_entity).rotation;
}

} // namespace

//------------------------------------------------------------------------------
std::string MvpDemo::description() const
{
    return "MVP stack demo: dynamic cubes with AABB physics, point light, "
           "Behavior spin, DebugDraw colliders, scene::saveScene JSON.";
}

//------------------------------------------------------------------------------
gpu::Status MvpDemo::setUp()
{
    GPU_TRY_ASSIGN(cube, assets::makeCube());
    GPU_TRY_ASSIGN(mesh_id, m_assets.addMesh("cube", std::move(cube)));
    m_cube_mesh = mesh_id;

    GPU_TRY_ASSIGN(lit_material, assets::makeLitMaterial());
    GPU_TRY_ASSIGN(material_id,
                   m_assets.addMaterial("lit", std::move(lit_material)));
    const assets::MaterialId lit_id = material_id;

    GPU_TRY_ASSIGN(
        red_id,
        m_assets.addMaterialInstance(
            "red", assets::MaterialInstance{ lit_id, Vector3f(0.9f, 0.2f, 0.2f) }));
    GPU_TRY_ASSIGN(
        blue_id,
        m_assets.addMaterialInstance(
            "blue", assets::MaterialInstance{ lit_id, Vector3f(0.2f, 0.5f, 0.95f) }));
    m_red = red_id;
    m_blue = blue_id;

    m_floor = m_world.create("Floor");
    m_world.transform(m_floor).position = Vector3f(0.0f, -0.5f, 0.0f);
    m_world.transform(m_floor).scale = Vector3f(40.0f, 1.0f, 40.0f);
    m_world.add(m_floor, world::MeshRenderer{ m_cube_mesh, m_blue });
    m_world.add(m_floor, world::RigidBody{ world::BodyType::Static, {}, 0.0f });
    m_world.add(m_floor, world::BoxCollider{ Vector3f(0.5f, 0.5f, 0.5f) });

    const std::array<Vector3f, 4u> starts{
        Vector3f(-4.0f, 8.0f, -2.0f),
        Vector3f(0.0f, 10.0f, 1.0f),
        Vector3f(3.0f, 12.0f, -1.0f),
        Vector3f(-2.0f, 14.0f, 3.0f)
    };
    for (Vector3f const& start : starts)
    {
        world::Entity cube = m_world.create("Cube");
        m_world.transform(cube).position = start;
        m_world.add(cube, world::MeshRenderer{ m_cube_mesh, m_red });
        m_world.add(cube, world::RigidBody{});
        m_world.add(cube, world::BoxCollider{ Vector3f(0.5f, 0.5f, 0.5f) });
        m_cubes.push_back(cube);
    }

    m_spinner = m_world.create("Spinner");
    m_world.transform(m_spinner).position = Vector3f(0.0f, 1.0f, -6.0f);
    m_world.add(m_spinner, world::MeshRenderer{ m_cube_mesh, m_blue });
    m_world.add(m_spinner, world::RigidBody{ world::BodyType::Kinematic });
    m_world.add(m_spinner, world::BoxCollider{ Vector3f(0.5f, 0.5f, 0.5f) });
    world::Behavior spin;
    spin.callback = spinBehavior;
    m_world.add(m_spinner, spin);

    m_sun = m_world.create("Sun");
    m_world.add(m_sun, world::DirectionalLight{ Vector3f(1.0f, 0.96f, 0.88f), 0.8f });
    m_world.transform(m_sun).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(-0.8f),
                             Vector3f(1.0f, 0.0f, 0.0f));

    m_lamp = m_world.create("Lamp");
    m_world.transform(m_lamp).position = Vector3f(4.0f, 6.0f, 2.0f);
    m_world.add(m_lamp, world::PointLight{ Vector3f(1.0f, 0.85f, 0.5f), 2.0f, 20.0f });

    m_camera = m_world.create("Camera");
    world::Camera camera;
    camera.fov_degrees = 55.0f;
    camera.near_plane = 0.2f;
    camera.far_plane = 200.0f;
    m_world.add(m_camera, camera);
    m_world.transform(m_camera).position = Vector3f(0.0f, 12.0f, 28.0f);

    m_scene.setActiveCamera(m_camera);
    m_scene.renderSettings().clear_color = Vector4f(0.04f, 0.06f, 0.1f, 1.0f);
    m_scene.environment().ambient = Vector3f(0.08f, 0.09f, 0.12f);

    m_scene_path = "/tmp/gloop_mvp_scene.json";
    m_world.update();
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status MvpDemo::draw(Frame const& p_frame)
{
    fillInputState(m_input, p_frame);
    world::OrbitController orbit;
    orbit.distance = 32.0f;
    orbit.target = Vector3f(0.0f, 2.0f, 0.0f);
    orbit.apply(m_world, m_camera, toCameraInput(m_input));

    world::updateBehaviors(m_world, p_frame.elapsed, m_input);
    m_events.clear();
    m_physics.step(m_world, p_frame.elapsed, &m_events);

    if (!m_saved && (p_frame.total > 0.15f))
    {
        GPU_TRY(scene::saveScene(m_scene, m_scene_path));
        m_saved = true;
    }

    GPU_TRY_ASSIGN(
        snapshot,
        render::Extractor::extract(m_scene, p_frame.width, p_frame.height));

    m_debug.clear();
    for (world::Entity cube : m_cubes)
    {
        if (world::BoxCollider const* collider =
                m_world.tryGet<world::BoxCollider>(cube))
        {
            assets::MeshAsset const* mesh = m_assets.mesh(m_cube_mesh);
            if (mesh != nullptr)
            {
                m_debug.box(mesh->local_bounds,
                          m_world.worldMatrix(cube),
                          Vector3f(1.0f, 0.8f, 0.2f));
            }
        }
    }

    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = m_scene.renderSettings().clear_color;
    desc.clear_depth = true;
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
    GPU_TRY(m_renderer.render(pass, snapshot, m_assets));
    GPU_TRY(m_debug.flush(snapshot.camera));
    return gpu::success();
}

} // namespace examples
