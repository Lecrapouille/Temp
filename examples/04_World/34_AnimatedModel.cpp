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

#include "04_World/34_AnimatedModel.hpp"

#include "Assets/Primitives.hpp"
#include "GPU/RenderPass.hpp"
#include "Math/Units.hpp"
#include "Render/Extractor.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/Light.hpp"
#include "World/Components/MeshRenderer.hpp"

#include <cmath>

using namespace units::literals;

namespace examples
{

namespace
{

Quatf standCylinderUp()
{
    return Quatf::fromAngleAxis(units::angle::degree_t(90.0),
                                Vector3f(1.0f, 0.0f, 0.0f));
}

Quatf layFlatOnGround()
{
    return Quatf::fromAngleAxis(units::angle::degree_t(-90.0),
                                Vector3f(1.0f, 0.0f, 0.0f));
}

} // namespace

//------------------------------------------------------------------------------
std::string AnimatedModel::description() const
{
    return "A walk-cycle model: hips and shoulders are unit joints, cylinders "
           "and a sphere hang underneath so a swing never squashes a limb. "
           "The root walks a circle; opposite-phase sin(total) drives the "
           "gait. Two walkers share the meshes. Right-drag is unused — the "
           "camera orbits so --check sees the stride.";
}

//------------------------------------------------------------------------------
gpu::Status AnimatedModel::makeWalker(Walker& p_walker,
                                      char const* p_name,
                                      float p_phase,
                                      float p_radius,
                                      assets::MaterialInstanceId p_shirt,
                                      assets::MaterialInstanceId p_limb)
{
    p_walker.phase = p_phase;
    p_walker.radius = p_radius;
    p_walker.root = m_world.create(p_name);

    auto hang = [&](world::Entity p_parent,
                    char const* p_name_part,
                    assets::MeshAssetId p_mesh,
                    assets::MaterialInstanceId p_material,
                    Vector3f const& p_offset,
                    Quatf const& p_rotation,
                    Vector3f const& p_scale) -> gpu::Status {
        world::Entity mesh = m_world.create(p_name_part);
        world::LocalTransform& local = m_world.transform(mesh);
        local.position = p_offset;
        local.rotation = p_rotation;
        local.scale = p_scale;
        m_world.add(mesh, world::MeshRenderer{ p_mesh, p_material });
        return m_world.setParent(mesh, p_parent);
    };

    constexpr float hip_y = 0.50f;
    constexpr float torso_h = 0.46f;
    constexpr float arm_h = 0.40f;
    constexpr float leg_h = 0.50f;

    p_walker.torso = m_world.create("Torso");
    m_world.transform(p_walker.torso).position = Vector3f(0.0f, hip_y, 0.0f);
    GPU_TRY(m_world.setParent(p_walker.torso, p_walker.root));
    GPU_TRY(hang(p_walker.torso, "TorsoMesh", m_cube_mesh, p_shirt,
                 Vector3f(0.0f, torso_h * 0.5f, 0.0f), Quatf{},
                 Vector3f(0.34f, torso_h, 0.20f)));

    p_walker.head = m_world.create("Head");
    m_world.transform(p_walker.head).position =
        Vector3f(0.0f, torso_h + 0.16f, 0.0f);
    GPU_TRY(m_world.setParent(p_walker.head, p_walker.torso));
    GPU_TRY(hang(p_walker.head, "HeadMesh", m_sphere_mesh, m_skin_mat,
                 Vector3f(0.0f, 0.0f, 0.0f), Quatf{}, Vector3f(1.0f)));

    auto makeArm = [&](world::Entity& p_joint,
                       char const* p_joint_name,
                       char const* p_mesh_name,
                       float p_x) -> gpu::Status {
        p_joint = m_world.create(p_joint_name);
        m_world.transform(p_joint).position =
            Vector3f(p_x, torso_h - 0.04f, 0.0f);
        GPU_TRY(m_world.setParent(p_joint, p_walker.torso));
        return hang(p_joint, p_mesh_name, m_limb_mesh, p_limb,
                    Vector3f(0.0f, -arm_h * 0.5f, 0.0f), standCylinderUp(),
                    Vector3f(0.90f, 0.90f, arm_h / 0.50f));
    };
    GPU_TRY(makeArm(p_walker.left_shoulder, "LeftShoulder", "LeftArm", -0.24f));
    GPU_TRY(makeArm(p_walker.right_shoulder, "RightShoulder", "RightArm", 0.24f));

    auto makeLeg = [&](world::Entity& p_joint,
                       char const* p_joint_name,
                       char const* p_mesh_name,
                       float p_x) -> gpu::Status {
        p_joint = m_world.create(p_joint_name);
        m_world.transform(p_joint).position = Vector3f(p_x, 0.0f, 0.0f);
        GPU_TRY(m_world.setParent(p_joint, p_walker.torso));
        GPU_TRY(hang(p_joint, p_mesh_name, m_limb_mesh, p_limb,
                     Vector3f(0.0f, -leg_h * 0.5f, 0.0f), standCylinderUp(),
                     Vector3f(1.15f, 1.15f, 1.0f)));
        world::Entity foot = m_world.create();
        m_world.transform(foot).position = Vector3f(0.0f, -leg_h, 0.08f);
        m_world.transform(foot).scale = Vector3f(0.12f, 0.06f, 0.20f);
        m_world.add(foot, world::MeshRenderer{ m_cube_mesh, p_limb });
        return m_world.setParent(foot, p_joint);
    };
    GPU_TRY(makeLeg(p_walker.left_hip, "LeftHip", "LeftLeg", -0.10f));
    GPU_TRY(makeLeg(p_walker.right_hip, "RightHip", "RightLeg", 0.10f));

    return gpu::success();
}

//------------------------------------------------------------------------------
void AnimatedModel::poseWalker(Walker& p_walker, float p_time)
{
    const float t = (p_time + p_walker.phase) * 2.4f;
    const float path = (p_time + p_walker.phase) * 0.55f;
    const float swing = std::sin(t);
    const float bob = std::abs(std::sin(t)) * 0.04f;

    world::LocalTransform& root = m_world.transform(p_walker.root);
    root.position = Vector3f(p_walker.radius * std::sin(path),
                             bob,
                             p_walker.radius * std::cos(path));
    root.rotation = Quatf::fromAngleAxis(units::angle::radian_t(path),
                                         Vector3f(0.0f, 1.0f, 0.0f));

    const Quatf leg_l = Quatf::fromAngleAxis(
        units::angle::radian_t(swing * 0.70f), Vector3f(1.0f, 0.0f, 0.0f));
    const Quatf leg_r = Quatf::fromAngleAxis(
        units::angle::radian_t(-swing * 0.70f), Vector3f(1.0f, 0.0f, 0.0f));
    const Quatf arm_l = Quatf::fromAngleAxis(
        units::angle::radian_t(-swing * 0.55f), Vector3f(1.0f, 0.0f, 0.0f));
    const Quatf arm_r = Quatf::fromAngleAxis(
        units::angle::radian_t(swing * 0.55f), Vector3f(1.0f, 0.0f, 0.0f));

    m_world.transform(p_walker.left_hip).rotation = leg_l;
    m_world.transform(p_walker.right_hip).rotation = leg_r;
    m_world.transform(p_walker.left_shoulder).rotation = arm_l;
    m_world.transform(p_walker.right_shoulder).rotation = arm_r;
    m_world.transform(p_walker.head).rotation = Quatf::fromAngleAxis(
        units::angle::radian_t(std::sin(t * 0.5f) * 0.15f),
        Vector3f(0.0f, 1.0f, 0.0f));
}

//------------------------------------------------------------------------------
gpu::Status AnimatedModel::setUp()
{
    GPU_TRY_ASSIGN(cube, assets::makeCube());
    GPU_TRY_ASSIGN(cube_id, m_assets.addMesh("cube", std::move(cube)));
    m_cube_mesh = cube_id;

    GPU_TRY_ASSIGN(sphere, assets::makeSphere(0.14f, 14u, 18u));
    GPU_TRY_ASSIGN(sphere_id, m_assets.addMesh("head", std::move(sphere)));
    m_sphere_mesh = sphere_id;

    GPU_TRY_ASSIGN(limb, assets::makeCylinder(0.055f, 0.50f, 14u));
    GPU_TRY_ASSIGN(limb_id, m_assets.addMesh("limb", std::move(limb)));
    m_limb_mesh = limb_id;

    GPU_TRY_ASSIGN(lit, assets::makeLitMaterial());
    GPU_TRY_ASSIGN(lit_id, m_assets.addMaterial("lit", std::move(lit)));

    auto instance = [&](char const* p_name,
                        Vector3f const& p_color,
                        assets::MaterialInstanceId& p_out) -> gpu::Status {
        GPU_TRY_ASSIGN(id,
                       m_assets.addMaterialInstance(
                           p_name, assets::MaterialInstance{ lit_id, p_color }));
        p_out = id;
        return gpu::success();
    };
    GPU_TRY(instance("ground", Vector3f(0.40f, 0.44f, 0.40f), m_ground_mat));
    GPU_TRY(instance("shirt_a", Vector3f(0.22f, 0.48f, 0.78f), m_shirt_a));
    GPU_TRY(instance("shirt_b", Vector3f(0.82f, 0.32f, 0.24f), m_shirt_b));
    GPU_TRY(instance("limb", Vector3f(0.28f, 0.30f, 0.36f), m_limb_mat));
    GPU_TRY(instance("skin", Vector3f(0.92f, 0.74f, 0.58f), m_skin_mat));

    GPU_TRY_ASSIGN(ground, assets::makePlane(14.0f, 14.0f, 1u, 1u));
    GPU_TRY_ASSIGN(ground_id, m_assets.addMesh("ground", std::move(ground)));
    world::Entity floor = m_world.create("Ground");
    m_world.transform(floor).rotation = layFlatOnGround();
    m_world.add(floor, world::MeshRenderer{ ground_id, m_ground_mat });

    GPU_TRY(makeWalker(m_walkers[0], "WalkerA", 0.0f, 3.1f, m_shirt_a,
                       m_limb_mat));
    GPU_TRY(makeWalker(m_walkers[1], "WalkerB", 1.7f, 2.2f, m_shirt_b,
                       m_limb_mat));

    m_sun = m_world.create("Sun");
    m_world.transform(m_sun).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(-0.85),
                             Vector3f(1.0f, 0.0f, 0.0f));
    m_world.add(m_sun,
                world::DirectionalLight{ Vector3f(1.0f, 0.97f, 0.90f), 1.1f });

    m_camera = m_world.create("Camera");
    world::Camera camera;
    camera.projection = world::Projection::Perspective;
    camera.fov_degrees = 45.0f;
    camera.near_plane = 0.1f;
    camera.far_plane = 80.0f;
    m_world.add(m_camera, camera);

    m_scene.setActiveCamera(m_camera);
    m_scene.renderSettings().clear_color = Vector4f(0.08f, 0.10f, 0.13f, 1.0f);
    m_scene.environment().ambient = Vector3f(0.20f, 0.20f, 0.22f);

    poseWalker(m_walkers[0], 0.0f);
    poseWalker(m_walkers[1], 0.0f);
    m_world.update();
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status AnimatedModel::draw(Frame const& p_frame)
{
    poseWalker(m_walkers[0], p_frame.total);
    poseWalker(m_walkers[1], p_frame.total);

    const float orbit = p_frame.total * 0.22f;
    const float radius = 8.5f;
    m_world.transform(m_camera).position =
        Vector3f(radius * std::sin(orbit), 3.6f, radius * std::cos(orbit));
    const Quatf yaw = Quatf::fromAngleAxis(units::angle::radian_t(orbit),
                                           Vector3f(0.0f, 1.0f, 0.0f));
    const Quatf pitch = Quatf::fromAngleAxis(
        units::angle::radian_t(-0.28), Vector3f(1.0f, 0.0f, 0.0f));
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
