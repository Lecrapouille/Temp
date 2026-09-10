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

#include "04_World/35_PhysicsSandbox.hpp"

#include "Assets/Primitives.hpp"
#include "GPU/RenderPass.hpp"
#include "Math/Units.hpp"
#include "Render/Extractor.hpp"
#include "World/Components/BoxCollider.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/Light.hpp"
#include "World/Components/MeshRenderer.hpp"
#include "World/Components/RigidBody.hpp"

#include <algorithm>
#include <array>
#include <cmath>

using namespace units::literals;

namespace examples
{

namespace
{

Quatf standZShapeUp()
{
    return Quatf::fromAngleAxis(units::angle::degree_t(90.0),
                                Vector3f(1.0f, 0.0f, 0.0f));
}

} // namespace

//------------------------------------------------------------------------------
std::string PhysicsSandbox::description() const
{
    return "physics::PhysicsWorld steps dynamic RigidBodies (gravity, AABB "
           "collisions) and writes positions back into LocalTransform. The "
           "floor is static, the pusher is kinematic. Cubes, spheres, cones "
           "and cylinders fall in; a collision event swaps the material for a "
           "frame. Bodies that leave the slab are respawned above.";
}

//------------------------------------------------------------------------------
gpu::Status PhysicsSandbox::spawnBody(Vector3f const& p_place,
                                      PropKind const& p_kind)
{
    world::Entity body = m_world.create(std::string(p_kind.name));
    m_world.transform(body).position = p_place;
    m_world.transform(body).rotation = p_kind.rotation;
    m_world.add(body, world::MeshRenderer{ p_kind.mesh, p_kind.material });
    m_world.add(body, world::RigidBody{});
    m_world.add(body, world::BoxCollider{ p_kind.half_extent });
    m_bodies.push_back(body);
    m_idle_mats.push_back(p_kind.material);
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status PhysicsSandbox::setUp()
{
    GPU_TRY_ASSIGN(cube, assets::makeCube());
    GPU_TRY_ASSIGN(cube_mesh, m_assets.addMesh("cube", std::move(cube)));
    m_cube_mesh = cube_mesh;

    GPU_TRY_ASSIGN(sphere, assets::makeSphere(0.5f, 20u, 24u));
    GPU_TRY_ASSIGN(sphere_mesh, m_assets.addMesh("sphere", std::move(sphere)));

    GPU_TRY_ASSIGN(cone, assets::makeCone(0.45f, 0.0f, 1.0f, 20u));
    GPU_TRY_ASSIGN(cone_mesh, m_assets.addMesh("cone", std::move(cone)));

    GPU_TRY_ASSIGN(cylinder, assets::makeCylinder(0.38f, 1.0f, 24u));
    GPU_TRY_ASSIGN(cylinder_mesh,
                   m_assets.addMesh("cylinder", std::move(cylinder)));

    GPU_TRY_ASSIGN(lit, assets::makeLitMaterial());
    GPU_TRY_ASSIGN(lit_id, m_assets.addMaterial("lit", std::move(lit)));

    auto tint =
        [&](char const* p_name,
            Vector3f const& p_color) -> gloop::Result<assets::MaterialInstanceId> {
        return m_assets.addMaterialInstance(
            p_name, assets::MaterialInstance{ lit_id, p_color });
    };

    GPU_TRY_ASSIGN(floor_id, tint("floor", Vector3f(0.38f, 0.40f, 0.44f)));
    GPU_TRY_ASSIGN(cube_id, tint("cube", Vector3f(0.86f, 0.42f, 0.18f)));
    GPU_TRY_ASSIGN(sphere_id, tint("sphere", Vector3f(0.28f, 0.72f, 0.42f)));
    GPU_TRY_ASSIGN(cone_id, tint("cone", Vector3f(0.72f, 0.38f, 0.82f)));
    GPU_TRY_ASSIGN(cylinder_id, tint("cylinder", Vector3f(0.22f, 0.70f, 0.74f)));
    GPU_TRY_ASSIGN(hit_id, tint("hit", Vector3f(0.98f, 0.92f, 0.35f)));
    GPU_TRY_ASSIGN(pusher_id, tint("pusher", Vector3f(0.25f, 0.62f, 0.88f)));
    m_floor_mat = floor_id;
    m_hit_mat = hit_id;
    m_pusher_mat = pusher_id;

    const Quatf stand = standZShapeUp();
    const std::array<PropKind, 4u> kinds{
        PropKind{ "Cube", cube_mesh, cube_id, Vector3f(0.5f, 0.5f, 0.5f), {} },
        PropKind{ "Sphere",
                  sphere_mesh,
                  sphere_id,
                  Vector3f(0.5f, 0.5f, 0.5f),
                  {} },
        PropKind{ "Cone",
                  cone_mesh,
                  cone_id,
                  Vector3f(0.45f, 0.5f, 0.45f),
                  stand },
        PropKind{ "Cylinder",
                  cylinder_mesh,
                  cylinder_id,
                  Vector3f(0.38f, 0.5f, 0.38f),
                  stand }
    };

    m_floor = m_world.create("Floor");
    m_world.transform(m_floor).position = Vector3f(0.0f, -0.5f, 0.0f);
    m_world.transform(m_floor).scale = Vector3f(14.0f, 1.0f, 14.0f);
    m_world.add(m_floor, world::MeshRenderer{ m_cube_mesh, m_floor_mat });
    m_world.add(m_floor, world::RigidBody{ world::BodyType::Static, {}, 0.0f });
    m_world.add(m_floor, world::BoxCollider{ Vector3f(0.5f, 0.5f, 0.5f) });

    m_pusher = m_world.create("Pusher");
    m_world.transform(m_pusher).position = Vector3f(0.0f, 0.35f, 0.0f);
    m_world.transform(m_pusher).scale = Vector3f(3.2f, 0.5f, 0.55f);
    m_world.add(m_pusher, world::MeshRenderer{ m_cube_mesh, m_pusher_mat });
    m_world.add(m_pusher, world::RigidBody{ world::BodyType::Kinematic });
    m_world.add(m_pusher, world::BoxCollider{ Vector3f(0.5f, 0.5f, 0.5f) });

    const std::array<Vector3f, 12u> starts{
        Vector3f(-1.6f, 6.0f, -1.1f), Vector3f(0.2f, 7.4f, 0.5f),
        Vector3f(1.6f, 8.8f, -0.7f),  Vector3f(-0.7f, 10.2f, 1.3f),
        Vector3f(0.9f, 11.5f, 0.1f),  Vector3f(-2.1f, 6.9f, 1.7f),
        Vector3f(2.3f, 8.2f, 1.2f),   Vector3f(-1.2f, 10.8f, -1.6f),
        Vector3f(0.4f, 12.6f, -1.9f), Vector3f(1.9f, 5.6f, -1.5f),
        Vector3f(-0.3f, 13.4f, 0.8f), Vector3f(1.1f, 14.2f, -0.4f)
    };
    for (std::size_t i = 0u; i < starts.size(); ++i)
    {
        GPU_TRY(spawnBody(starts[i], kinds[i % kinds.size()]));
    }

    m_sun = m_world.create("Sun");
    m_world.transform(m_sun).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(-0.75),
                             Vector3f(1.0f, 0.0f, 0.0f));
    m_world.add(m_sun,
                world::DirectionalLight{ Vector3f(1.0f, 0.96f, 0.88f), 1.05f });

    m_camera = m_world.create("Camera");
    world::Camera camera;
    camera.projection = world::Projection::Perspective;
    camera.fov_degrees = 50.0f;
    camera.near_plane = 0.15f;
    camera.far_plane = 120.0f;
    m_world.add(m_camera, camera);

    m_scene.setActiveCamera(m_camera);
    m_scene.renderSettings().clear_color = Vector4f(0.07f, 0.08f, 0.11f, 1.0f);
    m_scene.environment().ambient = Vector3f(0.16f, 0.16f, 0.18f);

    m_world.update();
    for (int i = 0; i < 50; ++i)
    {
        m_physics.step(m_world, 1.0f / 60.0f, nullptr);
    }
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status PhysicsSandbox::draw(Frame const& p_frame)
{
    const float dt = std::min(p_frame.elapsed, 1.0f / 30.0f);

    world::LocalTransform& pusher = m_world.transform(m_pusher);
    const Vector3f next_pusher(
        std::sin(p_frame.total * 1.1f) * 3.5f, 0.35f, 0.0f);
    if (world::RigidBody* body = m_world.tryGet<world::RigidBody>(m_pusher))
    {
        body->velocity =
            (dt > 1.0e-6f) ? ((next_pusher - pusher.position) / dt)
                           : Vector3f(0.0f, 0.0f, 0.0f);
    }
    pusher.position = next_pusher;

    m_events.clear();
    m_physics.step(m_world, dt, &m_events);

    for (std::size_t i = 0u; i < m_bodies.size(); ++i)
    {
        if (world::MeshRenderer* renderer =
                m_world.tryGet<world::MeshRenderer>(m_bodies[i]))
        {
            renderer->material_instance = m_idle_mats[i];
        }
    }
    for (world::Event const& event : m_events.events())
    {
        auto paint = [&](world::Entity p_entity) {
            if (world::MeshRenderer* renderer =
                    m_world.tryGet<world::MeshRenderer>(p_entity))
            {
                if ((p_entity != m_floor) && (p_entity != m_pusher))
                {
                    renderer->material_instance = m_hit_mat;
                }
            }
        };
        paint(event.a);
        paint(event.b);
    }

    for (world::Entity body : m_bodies)
    {
        world::LocalTransform& local = m_world.transform(body);
        const bool lost = (local.position.y < -4.0f) ||
                          (std::abs(local.position.x) > 9.0f) ||
                          (std::abs(local.position.z) > 9.0f);
        if (lost)
        {
            local.position = Vector3f(0.0f, 10.0f, 0.0f);
            if (world::RigidBody* rigid = m_world.tryGet<world::RigidBody>(body))
            {
                rigid->velocity = Vector3f(0.0f, 0.0f, 0.0f);
            }
        }
    }

    const float orbit = p_frame.total * 0.25f;
    const float radius = 16.0f;
    m_world.transform(m_camera).position =
        Vector3f(radius * std::sin(orbit), 9.0f, radius * std::cos(orbit));
    const Quatf yaw = Quatf::fromAngleAxis(units::angle::radian_t(orbit),
                                           Vector3f(0.0f, 1.0f, 0.0f));
    const Quatf pitch = Quatf::fromAngleAxis(
        units::angle::radian_t(-0.40), Vector3f(1.0f, 0.0f, 0.0f));
    m_world.transform(m_camera).rotation = yaw * pitch;

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
