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

#include "04_World/19_SplitViews.hpp"

#include "Assets/Primitives.hpp"
#include "GPU/RenderPass.hpp"
#include "Math/Quaternion.hpp"
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

//! \brief How many towers ring the pillar. Enough to make the top-down view
//! read as a clock; not so many that a screenshot cannot count them.
constexpr std::size_t TOWER_COUNT = 8u;
constexpr float RING_RADIUS = 40.0f;
constexpr float SCENE_HALF_EXTENT = 60.0f; // used by the top-down camera

} // namespace

//------------------------------------------------------------------------------
SplitViews::SplitViews() = default;

//------------------------------------------------------------------------------
std::string SplitViews::description() const
{
    return "One World, one AssetManager, and two Scenes: a perspective camera "
           "orbiting on the left, an orthographic top-down camera on the "
           "right. Two RenderPass::begin() calls run one after the other; "
           "each has its own viewport thanks to PassDesc x/y/width/height, "
           "and each renders the same simulation from its own point of view. "
           "Adding a third view is one more Scene and one more pass.";
}

//------------------------------------------------------------------------------
gpu::Status SplitViews::setUp()
{
    // Shared assets. Everything in both views comes from these three ids.
    GPU_TRY_ASSIGN(cube, assets::makeCube());
    GPU_TRY_ASSIGN(mesh_id, m_assets.addMesh("cube", std::move(cube)));
    m_cube_mesh = mesh_id;

    GPU_TRY_ASSIGN(lit, assets::makeLitMaterial());
    GPU_TRY_ASSIGN(material_id, m_assets.addMaterial("lit", std::move(lit)));
    m_lit_material = material_id;

    GPU_TRY_ASSIGN(
        pillar_id,
        m_assets.addMaterialInstance(
            "pillar", assets::MaterialInstance{
                          m_lit_material, Vector3f(0.85f, 0.55f, 0.35f) }));
    m_pillar_color = pillar_id;
    GPU_TRY_ASSIGN(
        tower_id,
        m_assets.addMaterialInstance(
            "tower", assets::MaterialInstance{
                         m_lit_material, Vector3f(0.55f, 0.60f, 0.70f) }));
    m_tower_color = tower_id;
    GPU_TRY_ASSIGN(
        ground_id,
        m_assets.addMaterialInstance(
            "ground", assets::MaterialInstance{
                          m_lit_material, Vector3f(0.20f, 0.28f, 0.20f) }));
    m_ground_color = ground_id;

    // The scene. Root -> {ground, pillar, {towers...}}.
    m_root = m_world.create("Root");

    // Flat ground. A very thin, very wide cube.
    world::Entity ground = m_world.create("Ground");
    m_world.transform(ground).position = Vector3f(0.0f, -0.5f, 0.0f);
    m_world.transform(ground).scale = Vector3f(140.0f, 1.0f, 140.0f);
    m_world.add(ground, world::MeshRenderer{ m_cube_mesh, m_ground_color });
    GPU_TRY(m_world.setParent(ground, m_root));

    // A pillar in the middle that turns, so the two views clearly refer to
    // the same simulation.
    m_pillar = m_world.create("Pillar");
    m_world.transform(m_pillar).position = Vector3f(0.0f, 8.0f, 0.0f);
    m_world.transform(m_pillar).scale = Vector3f(4.0f, 16.0f, 4.0f);
    m_world.add(m_pillar, world::MeshRenderer{ m_cube_mesh, m_pillar_color });
    GPU_TRY(m_world.setParent(m_pillar, m_root));

    // Towers on a ring around the pillar.
    m_towers.reserve(TOWER_COUNT);
    for (std::size_t i = 0u; i < TOWER_COUNT; ++i)
    {
        const float a = (2.0f * 3.14159265f) *
                        (static_cast<float>(i) / static_cast<float>(TOWER_COUNT));
        world::Entity tower = m_world.create();
        m_world.transform(tower).position =
            Vector3f(RING_RADIUS * std::cos(a), 6.0f, RING_RADIUS * std::sin(a));
        m_world.transform(tower).scale = Vector3f(4.0f, 12.0f, 4.0f);
        m_world.add(tower, world::MeshRenderer{ m_cube_mesh, m_tower_color });
        GPU_TRY(m_world.setParent(tower, m_root));
        m_towers.push_back(tower);
    }

    // One sun.
    m_sun = m_world.create("Sun");
    m_world.transform(m_sun).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(-1.05),
                             Vector3f(1.0f, 0.0f, 0.0f));
    m_world.add(m_sun,
                world::DirectionalLight{ Vector3f(1.0f, 0.97f, 0.9f), 1.0f });

    // Perspective camera. Position and rotation are rewritten each frame.
    m_camera_perspective = m_world.create("PerspectiveCamera");
    world::Camera persp;
    persp.projection = world::Projection::Perspective;
    persp.fov_degrees = 55.0f;
    persp.near_plane = 1.0f;
    persp.far_plane = 400.0f;
    m_world.add(m_camera_perspective, persp);

    // Top-down orthographic camera. Its transform never moves: fixed above
    // the origin, looking straight down. The projection is what determines
    // the width of the view; ortho_half_height is what to set so the whole
    // ring fits.
    m_camera_top = m_world.create("TopCamera");
    m_world.transform(m_camera_top).position = Vector3f(0.0f, 120.0f, 0.0f);
    // Look straight down: rotate -90° around X so the local -Z (view axis)
    // ends up pointing along -Y.
    m_world.transform(m_camera_top).rotation = Quatf::fromAngleAxis(
        units::angle::radian_t(-1.5707963), Vector3f(1.0f, 0.0f, 0.0f));
    world::Camera top;
    top.projection = world::Projection::Orthographic;
    top.ortho_half_height = SCENE_HALF_EXTENT;
    top.near_plane = 1.0f;
    top.far_plane = 300.0f;
    m_world.add(m_camera_top, top);

    // Both Scenes reference the same World and the same AssetManager. Each
    // just picks a different camera and a different clear colour.
    m_scene_perspective.emplace(m_world, m_assets);
    m_scene_perspective->setActiveCamera(m_camera_perspective);
    m_scene_perspective->renderSettings().clear_color =
        Vector4f(0.05f, 0.08f, 0.20f, 1.0f);
    m_scene_perspective->environment().ambient =
        Vector3f(0.15f, 0.15f, 0.18f);

    m_scene_top.emplace(m_world, m_assets);
    m_scene_top->setActiveCamera(m_camera_top);
    m_scene_top->renderSettings().clear_color =
        Vector4f(0.10f, 0.10f, 0.10f, 1.0f);
    m_scene_top->environment().ambient = Vector3f(0.15f, 0.15f, 0.18f);

    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status SplitViews::draw(Frame const& p_frame)
{
    // Animate the world: the pillar turns steadily around Y.
    m_world.transform(m_pillar).rotation = Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(p_frame.total * 0.6f)),
        Vector3f(0.0f, 1.0f, 0.0f));

    // Move the perspective camera on a slow orbit at ground level, aimed at
    // the pillar. Yaw around Y then a small downward pitch, exactly the same
    // trick 18_ManyCubes uses; nothing here needs a lookAt matrix.
    const float t = p_frame.total * 0.15f;
    const float radius = 90.0f;
    const float height = 25.0f;
    m_world.transform(m_camera_perspective).position =
        Vector3f(radius * std::sin(t), height, radius * std::cos(t));
    const Quatf q_yaw = Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(t)),
        Vector3f(0.0f, 1.0f, 0.0f));
    const Quatf q_pitch = Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(-std::atan2(height, radius))),
        Vector3f(1.0f, 0.0f, 0.0f));
    m_world.transform(m_camera_perspective).rotation = q_yaw * q_pitch;

    // Update the World once. Both extractions read the same world matrices.
    m_world.update();

    // Split the window into two equal halves. The left half gets the
    // perspective view, the right half gets the top-down view. Each pass has
    // its own viewport and its own clear colour, so the two never share
    // state.
    const std::uint32_t half_w = p_frame.width / 2u;
    const std::uint32_t right_w = p_frame.width - half_w;
    const std::uint32_t height_px = p_frame.height;

    // Left view: perspective.
    {
        GPU_TRY_ASSIGN(
            snapshot,
            render::Extractor::extract(*m_scene_perspective, half_w, height_px));
        gpu::PassDesc desc;
        desc.x = 0u;
        desc.y = 0u;
        desc.width = half_w;
        desc.height = height_px;
        desc.color = m_scene_perspective->renderSettings().clear_color;
        desc.clear_depth = true;
        GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
        GPU_TRY(m_renderer.render(pass, snapshot, m_assets));
    }

    // Right view: orthographic top-down. A second pass starts after the
    // first has ended (the object went out of scope), which is why the two
    // are in separate blocks.
    {
        GPU_TRY_ASSIGN(
            snapshot,
            render::Extractor::extract(*m_scene_top, right_w, height_px));
        gpu::PassDesc desc;
        desc.x = half_w;
        desc.y = 0u;
        desc.width = right_w;
        desc.height = height_px;
        desc.color = m_scene_top->renderSettings().clear_color;
        desc.clear_depth = true;
        GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
        GPU_TRY(m_renderer.render(pass, snapshot, m_assets));
    }

    return gpu::success();
}

} // namespace examples
