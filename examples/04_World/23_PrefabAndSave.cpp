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

#include "04_World/23_PrefabAndSave.hpp"

#include "Assets/Primitives.hpp"
#include "Assets/Prefabs.hpp"
#include "GPU/RenderPass.hpp"
#include "Math/Units.hpp"
#include "Render/Extractor.hpp"
#include "Scene/SceneSerializer.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/Light.hpp"
#include "World/PrefabInstantiate.hpp"

#include <cmath>
#include <cstdio>

using namespace units::literals;

namespace examples
{

//------------------------------------------------------------------------------
std::string PrefabAndSave::description() const
{
    return "makeRobotPrefab() bakes the 17_MovingRobot hierarchy as an asset. "
           "instantiate() spawns it three times; animation still writes local "
           "transforms on the joints. After a few frames the World is saved "
           "to /tmp/gloop_prefab_scene.json with asset names, not gpu ids.";
}

//------------------------------------------------------------------------------
gpu::Status PrefabAndSave::setUp()
{
    GPU_TRY_ASSIGN(cube, assets::makeCube());
    GPU_TRY_ASSIGN(mesh_id, m_assets.addMesh("cube", std::move(cube)));

    GPU_TRY_ASSIGN(lit_material, assets::makeLitMaterial());
    GPU_TRY_ASSIGN(material_id,
                   m_assets.addMaterial("lit", std::move(lit_material)));
    const assets::MaterialId lit_id = material_id;

    GPU_TRY_ASSIGN(
        wood_id,
        m_assets.addMaterialInstance(
            "wood",
            assets::MaterialInstance{ lit_id, Vector3f(0.62f, 0.42f, 0.24f) }));
    GPU_TRY_ASSIGN(
        dark_id,
        m_assets.addMaterialInstance(
            "dark",
            assets::MaterialInstance{ lit_id, Vector3f(0.35f, 0.24f, 0.14f) }));
    GPU_TRY_ASSIGN(
        light_id,
        m_assets.addMaterialInstance(
            "light",
            assets::MaterialInstance{ lit_id, Vector3f(0.92f, 0.90f, 0.82f) }));
    (void)wood_id;
    (void)dark_id;
    (void)light_id;

    GPU_TRY_ASSIGN(prefab_id,
                   m_assets.addPrefab("robot", assets::makeRobotPrefab()));
    m_robot_prefab = prefab_id;

    const std::array<Vector3f, 3u> places{
        Vector3f(-40.0f, 0.0f, 0.0f),
        Vector3f(0.0f, 0.0f, 0.0f),
        Vector3f(40.0f, 0.0f, 0.0f)
    };
    const std::array<float, 3u> phases{ 0.0f, 1.2f, 2.4f };

    for (std::size_t i = 0u; i < m_robots.size(); ++i)
    {
        world::LocalTransform offset;
        offset.position = places[i];
        GPU_TRY_ASSIGN(root,
                       world::instantiate(m_world,
                                          m_assets,
                                          m_robot_prefab,
                                          {},
                                          offset));
        m_robots[i].root = root;
        m_robots[i].phase = phases[i];
        m_robots[i].left_shoulder =
            m_world.find(root, "Body/LeftShoulder");
        m_robots[i].right_shoulder =
            m_world.find(root, "Body/RightShoulder");
        m_robots[i].head = m_world.find(root, "Body/Head");
    }

    m_sun = m_world.create("Sun");
    m_world.transform(m_sun).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(-0.9),
                             Vector3f(1.0f, 0.0f, 0.0f));
    m_world.add(m_sun,
                world::DirectionalLight{ Vector3f(1.0f, 0.96f, 0.88f), 1.0f });

    m_camera = m_world.create("Camera");
    world::Camera camera;
    camera.fov_degrees = 55.0f;
    camera.near_plane = 0.2f;
    camera.far_plane = 500.0f;
    m_world.add(m_camera, camera);
    m_world.transform(m_camera).position = Vector3f(0.0f, 35.0f, 120.0f);
    m_world.transform(m_camera).rotation = Quatf::fromAngleAxis(
        units::angle::radian_t(-0.18), Vector3f(1.0f, 0.0f, 0.0f));

    m_scene.setActiveCamera(m_camera);
    m_scene.renderSettings().clear_color = Vector4f(0.05f, 0.07f, 0.12f, 1.0f);
    m_scene.environment().ambient = Vector3f(0.14f, 0.15f, 0.18f);

    m_scene_path = "/tmp/gloop_prefab_scene.json";
    m_world.update();
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status PrefabAndSave::draw(Frame const& p_frame)
{
    const float t = p_frame.total;
    for (RobotHandles& robot : m_robots)
    {
        const float phase = t + robot.phase;
        const float swing = std::sin(phase * 1.8f) * 0.55f;
        m_world.transform(robot.left_shoulder).rotation =
            Quatf::fromAngleAxis(units::angle::radian_t(swing),
                                 Vector3f(0.0f, 0.0f, 1.0f));
        m_world.transform(robot.right_shoulder).rotation =
            Quatf::fromAngleAxis(units::angle::radian_t(-swing),
                                 Vector3f(0.0f, 0.0f, 1.0f));
        m_world.transform(robot.head).rotation =
            Quatf::fromAngleAxis(units::angle::radian_t(std::sin(phase) * 0.25f),
                                 Vector3f(0.0f, 1.0f, 0.0f));
    }

    if (!m_saved && (p_frame.total > 0.1f))
    {
        GPU_TRY(scene::save(m_world, m_assets, m_scene_path));
        m_saved = true;
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
