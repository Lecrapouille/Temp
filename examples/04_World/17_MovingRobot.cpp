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

#include "04_World/17_MovingRobot.hpp"

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

//------------------------------------------------------------------------------
//! \brief Robot silhouette.
//!
//! Grouping the sizes here keeps the layout of a robot in one place: changing
//! the arm size only has to be done in one line, and the offsets that place
//! the arm at the shoulder derive from those sizes.
struct Silhouette
{
    Vector3f body{ 20.0f, 30.0f, 10.0f };
    Vector3f head{ 10.0f, 10.0f, 10.0f };
    Vector3f arm{ 6.0f, 24.0f, 6.0f };
    Vector3f leg{ 6.0f, 26.0f, 6.0f };
};

} // namespace

//------------------------------------------------------------------------------
std::string MovingRobot::description() const
{
    return "A World is a tree of transforms, not a tree of draw callbacks. "
           "Three robots share one cube mesh and one lit material; the joints "
           "are posed each frame from sin(total_time), so the animation is "
           "deterministic and never drifts. Extraction freezes the World into "
           "a snapshot, the Renderer culls, sorts by material and draws. The "
           "World itself never touches gpu::.";
}

//------------------------------------------------------------------------------
gpu::Status MovingRobot::makeRobot(Robot& p_robot,
                                   char const* p_name,
                                   Vector3f const& p_place,
                                   float p_phase)
{
    // A part is: a spatial joint (no mesh, scale 1) plus a MeshRenderer child
    // that carries the visible cube and its per-part scale. Joints between
    // parents and meshes are how scale can be inherited by the standard TRS
    // composition without a giant parent squashing its children: only the
    // MeshRenderer child has a scale.
    const Silhouette S;
    const float body_half_y = S.body.y * 0.5f;
    const float arm_half_y = S.arm.y * 0.5f;
    const float leg_half_y = S.leg.y * 0.5f;

    p_robot.phase = p_phase;
    p_robot.root = m_world.create(p_name);
    m_world.transform(p_robot.root).position = p_place;

    // Helper that hangs a "mesh child" carrying its own scale under a joint.
    auto meshChild = [&](world::Entity p_parent,
                         char const* p_child_name,
                         assets::MaterialInstanceId p_material,
                         Vector3f const& p_size,
                         Vector3f const& p_offset) -> gpu::Result<world::Entity>
    {
        world::Entity mesh = m_world.create(p_child_name);
        m_world.transform(mesh).position = p_offset;
        m_world.transform(mesh).scale = p_size;
        m_world.add(mesh, world::MeshRenderer{ m_cube_mesh, p_material });
        GPU_TRY(m_world.setParent(mesh, p_parent));
        return mesh;
    };

    // Body joint: sits at leg height so the feet end up at y=0 in world.
    p_robot.body = m_world.create("Body");
    m_world.transform(p_robot.body).position =
        Vector3f(0.0f, body_half_y + S.leg.y, 0.0f);
    GPU_TRY(m_world.setParent(p_robot.body, p_robot.root));
    GPU_TRY(meshChild(p_robot.body, "BodyMesh", m_wood, S.body,
                      Vector3f(0.0f)));

    // Head joint on top of the body, with its mesh cube centred on it.
    p_robot.head = m_world.create("Head");
    m_world.transform(p_robot.head).position =
        Vector3f(0.0f, body_half_y + (S.head.y * 0.5f), 0.0f);
    GPU_TRY(m_world.setParent(p_robot.head, p_robot.body));
    GPU_TRY(meshChild(p_robot.head, "HeadMesh", m_light, S.head,
                      Vector3f(0.0f)));

    // Shoulders: a joint at each shoulder. The mesh child hangs half an arm
    // below its joint, so rotating the joint around Z makes the arm pivot
    // around the shoulder (not around its middle). The shoulder joints keep
    // scale 1; only the mesh child carries the arm size.
    p_robot.left_shoulder = m_world.create("LeftShoulder");
    m_world.transform(p_robot.left_shoulder).position =
        Vector3f(-((S.body.x * 0.5f) + (S.arm.x * 0.5f)), body_half_y, 0.0f);
    GPU_TRY(m_world.setParent(p_robot.left_shoulder, p_robot.body));
    GPU_TRY(meshChild(p_robot.left_shoulder, "LeftArmMesh", m_dark,
                      S.arm, Vector3f(0.0f, -arm_half_y, 0.0f)));

    p_robot.right_shoulder = m_world.create("RightShoulder");
    m_world.transform(p_robot.right_shoulder).position =
        Vector3f(((S.body.x * 0.5f) + (S.arm.x * 0.5f)), body_half_y, 0.0f);
    GPU_TRY(m_world.setParent(p_robot.right_shoulder, p_robot.body));
    GPU_TRY(meshChild(p_robot.right_shoulder, "RightArmMesh", m_dark,
                      S.arm, Vector3f(0.0f, -arm_half_y, 0.0f)));

    // Legs are not animated separately, so a plain mesh under the body with
    // a per-mesh scale is enough. Offset by half the leg so the feet touch
    // y=0 in world.
    world::Entity left_leg = m_world.create("LeftLeg");
    m_world.transform(left_leg).position =
        Vector3f(-(S.body.x * 0.25f), -(body_half_y + leg_half_y), 0.0f);
    m_world.transform(left_leg).scale = S.leg;
    m_world.add(left_leg, world::MeshRenderer{ m_cube_mesh, m_dark });
    GPU_TRY(m_world.setParent(left_leg, p_robot.body));

    world::Entity right_leg = m_world.create("RightLeg");
    m_world.transform(right_leg).position =
        Vector3f((S.body.x * 0.25f), -(body_half_y + leg_half_y), 0.0f);
    m_world.transform(right_leg).scale = S.leg;
    m_world.add(right_leg, world::MeshRenderer{ m_cube_mesh, m_dark });
    GPU_TRY(m_world.setParent(right_leg, p_robot.body));

    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status MovingRobot::setUp()
{
    // Assets: build the shared cube and the shared lit material, register
    // them in the AssetManager, then make one MaterialInstance per colour.
    GPU_TRY_ASSIGN(cube, assets::makeCube());
    GPU_TRY_ASSIGN(mesh_id, m_assets.addMesh("cube", std::move(cube)));
    m_cube_mesh = mesh_id;

    GPU_TRY_ASSIGN(lit, assets::makeLitMaterial());
    GPU_TRY_ASSIGN(material_id, m_assets.addMaterial("lit", std::move(lit)));
    m_lit_material = material_id;

    GPU_TRY_ASSIGN(
        wood_id,
        m_assets.addMaterialInstance(
            "wood",
            assets::MaterialInstance{ m_lit_material,
                                      Vector3f(0.62f, 0.42f, 0.24f) }));
    m_wood = wood_id;
    GPU_TRY_ASSIGN(
        dark_id,
        m_assets.addMaterialInstance(
            "dark",
            assets::MaterialInstance{ m_lit_material,
                                      Vector3f(0.35f, 0.24f, 0.14f) }));
    m_dark = dark_id;
    GPU_TRY_ASSIGN(
        light_id,
        m_assets.addMaterialInstance(
            "light",
            assets::MaterialInstance{ m_lit_material,
                                      Vector3f(0.85f, 0.66f, 0.42f) }));
    m_light = light_id;

    // The scene: one root, three robots hanging under it, one sun, one
    // camera. The sun is a directional light entity; its forward axis is the
    // direction the shader reads, so rotating the entity aims it.
    m_scene_root = m_world.create("Root");
    const std::array<char const*, 3u> robot_names{ "CubicRobot1",
                                                   "CubicRobot2",
                                                   "CubicRobot3" };
    for (std::size_t i = 0u; i < m_robots.size(); ++i)
    {
        const float x = (static_cast<float>(i) - 1.0f) * 34.0f;
        GPU_TRY(makeRobot(m_robots[i],
                          robot_names[i],
                          Vector3f(x, 0.0f, 0.0f),
                          static_cast<float>(i) * 0.7f));
        GPU_TRY(m_world.setParent(m_robots[i].root, m_scene_root));
    }

    // Aim the sun down and slightly forward. The lit shader treats lightDir
    // as "the direction the photons travel", so this is the axis the sun
    // shines along, not "the direction the surface looks at the sun".
    m_sun = m_world.create("Sun");
    m_world.transform(m_sun).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(-1.0472),
                             Vector3f(1.0f, 0.0f, 0.0f));
    m_world.add(m_sun,
                world::DirectionalLight{ Vector3f(1.0f, 0.98f, 0.92f), 1.0f });

    // The camera is an Entity of the World, placed the way any other spatial
    // entity is: its transform is its pose. Position (0, 45, 120) with a
    // small downward pitch centres the row of robots vertically at their
    // visual middle.
    m_camera = m_world.create("Camera");
    m_world.transform(m_camera).position = Vector3f(0.0f, 45.0f, 120.0f);
    m_world.transform(m_camera).rotation = Quatf::fromAngleAxis(
        units::angle::radian_t(-0.15), Vector3f(1.0f, 0.0f, 0.0f));
    world::Camera camera;
    camera.projection = world::Projection::Perspective;
    camera.fov_degrees = 60.0f;
    camera.near_plane = 1.0f;
    camera.far_plane = 1000.0f;
    m_world.add(m_camera, camera);

    m_scene.setActiveCamera(m_camera);
    m_scene.renderSettings().clear_color = Vector4f(0.05f, 0.08f, 0.20f, 1.0f);
    m_scene.environment().ambient = Vector3f(0.15f, 0.15f, 0.18f);

    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status MovingRobot::draw(Frame const& p_frame)
{
    // Animation is deterministic: each frame the joints are rewritten from
    // sin/cos of the total elapsed time. There is no per-frame accumulation,
    // so nothing can drift out of frame after a few thousand frames and the
    // arms never end up pointing along the view axis where they would look
    // like a small square vanishing into the body.
    //
    // Two small ideas here:
    // - The body turns steadily around Y: fromAngleAxis(t * ω, Y). No sin;
    //   the body is not swinging, it is turning.
    // - Arms and head oscillate: their rotation angle is A * sin(t * ω + φ).
    //   The right shoulder uses -sin so both arms swing in opposition.
    const float t = p_frame.total;

    // Radians per second and swing amplitudes, chosen so nothing crosses the
    // body: arm swing stays below 55°.
    const float body_speed = 0.6f;          // rad / s
    const float head_speed = 1.4f;          // rad / s
    const float arm_speed = 3.2f;           // rad / s
    const units::angle::radian_t head_amp{ 0.5 };    // ~29°
    const units::angle::radian_t arm_amp{ 0.95 };    // ~54°

    for (Robot& robot : m_robots)
    {
        const double local_t = static_cast<double>(t + robot.phase);
        const units::angle::radian_t body_angle{
            static_cast<double>(body_speed) * local_t };
        const units::angle::radian_t head_angle{
            head_amp.to<double>() *
            std::sin(static_cast<double>(head_speed) * local_t) };
        const units::angle::radian_t arm_angle{
            arm_amp.to<double>() *
            std::sin(static_cast<double>(arm_speed) * local_t) };

        // Body: full spin around Y.
        m_world.transform(robot.root).rotation =
            Quatf::fromAngleAxis(body_angle, Vector3f(0.0f, 1.0f, 0.0f));
        // Head: small oscillation around Y (left-right glance).
        m_world.transform(robot.head).rotation =
            Quatf::fromAngleAxis(head_angle, Vector3f(0.0f, 1.0f, 0.0f));
        // Shoulders: opposite swings around X (walk-cycle style). The mesh
        // child hangs below the joint, so this pivots the arm around the
        // shoulder point.
        m_world.transform(robot.left_shoulder).rotation =
            Quatf::fromAngleAxis(arm_angle, Vector3f(1.0f, 0.0f, 0.0f));
        m_world.transform(robot.right_shoulder).rotation =
            Quatf::fromAngleAxis(-arm_angle, Vector3f(1.0f, 0.0f, 0.0f));
    }

    // The frame, in the target order: update the World, extract a snapshot,
    // open the pass, render.
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
