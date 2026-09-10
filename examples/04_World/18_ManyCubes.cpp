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

#include "04_World/18_ManyCubes.hpp"

#include "Assets/Primitives.hpp"
#include "GPU/RenderPass.hpp"
#include "Math/Quaternion.hpp"
#include "Math/Units.hpp"
#include "Render/Extractor.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/Light.hpp"
#include "World/Components/MeshRenderer.hpp"

#include <array>
#include <cmath>
#include <string>

using namespace units::literals;

namespace examples
{

//------------------------------------------------------------------------------
std::string ManyCubes::description() const
{
    return "One mesh, one program, five material instances, and just under "
           "two thousand entities. The Renderer's queue sorts by material so "
           "cubes of the same colour draw together, and the Extractor's "
           "frustum culling drops the ones off-screen as the camera orbits. "
           "The draw-call count in the overlay is the number of survivors, "
           "not the count in the World.";
}

//------------------------------------------------------------------------------
gpu::Status ManyCubes::setUp()
{
    GPU_TRY_ASSIGN(cube_asset, assets::makeCube());
    GPU_TRY_ASSIGN(mesh_id,
                   m_assets.addMesh("cube", std::move(cube_asset)));
    m_cube_mesh = mesh_id;

    GPU_TRY_ASSIGN(lit, assets::makeLitMaterial());
    GPU_TRY_ASSIGN(material_id, m_assets.addMaterial("lit", std::move(lit)));
    m_lit_material = material_id;

    // Five colours. A blue-through-orange palette so the tinting of the light
    // stays legible on every one.
    const std::array<Vector3f, PALETTE_SIZE> colors{
        Vector3f(0.35f, 0.55f, 0.85f), // sky
        Vector3f(0.65f, 0.75f, 0.35f), // olive
        Vector3f(0.85f, 0.55f, 0.35f), // orange
        Vector3f(0.80f, 0.35f, 0.55f), // rose
        Vector3f(0.55f, 0.35f, 0.75f), // violet
    };
    const std::array<char const*, PALETTE_SIZE> names{
        "sky", "olive", "orange", "rose", "violet" };
    for (std::size_t i = 0u; i < PALETTE_SIZE; ++i)
    {
        GPU_TRY_ASSIGN(
            id,
            m_assets.addMaterialInstance(
                names[i], assets::MaterialInstance{ m_lit_material, colors[i] }));
        m_palette[i] = id;
    }

    // A grid of cubes centred on the origin. Colour is assigned by
    // (x + y + z) mod 5 rather than by any regular slice, so a naive draw
    // order would jump between materials all the time. The queue's sort by
    // material undoes that.
    m_root = m_world.create("Root");
    m_cubes.reserve(CUBE_COUNT);

    const float spacing = 4.0f;
    const float offset = -0.5f * static_cast<float>(GRID - 1u) * spacing;
    for (std::size_t x = 0u; x < GRID; ++x)
    {
        for (std::size_t y = 0u; y < GRID; ++y)
        {
            for (std::size_t z = 0u; z < GRID; ++z)
            {
                world::Entity cube = m_world.create();
                m_world.transform(cube).position = Vector3f(
                    offset + (spacing * static_cast<float>(x)),
                    offset + (spacing * static_cast<float>(y)),
                    offset + (spacing * static_cast<float>(z)));
                // Unit cube, slightly smaller than the spacing.
                m_world.transform(cube).scale = Vector3f(2.4f);
                const std::size_t palette_index = (x + y + z) % PALETTE_SIZE;
                m_world.add(cube, world::MeshRenderer{
                                      m_cube_mesh, m_palette[palette_index] });
                GPU_TRY(m_world.setParent(cube, m_root));
                m_cubes.push_back(cube);
            }
        }
    }

    // One sun, aimed down-front, colour of a low sky.
    m_sun = m_world.create("Sun");
    m_world.transform(m_sun).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(-0.9),
                             Vector3f(1.0f, 0.0f, 0.0f));
    m_world.add(m_sun,
                world::DirectionalLight{ Vector3f(1.0f, 0.95f, 0.85f), 1.0f });

    // The camera. Position and rotation are rewritten every frame by draw();
    // this is just the initial pose, the Camera component is what stays.
    m_camera = m_world.create("Camera");
    world::Camera camera;
    camera.projection = world::Projection::Perspective;
    camera.fov_degrees = 55.0f;
    camera.near_plane = 1.0f;
    camera.far_plane = 400.0f;
    m_world.add(m_camera, camera);

    m_scene.setActiveCamera(m_camera);
    m_scene.renderSettings().clear_color = Vector4f(0.03f, 0.05f, 0.10f, 1.0f);
    m_scene.renderSettings().frustum_culling = true;
    m_scene.environment().ambient = Vector3f(0.12f, 0.13f, 0.17f);

    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status ManyCubes::draw(Frame const& p_frame)
{
    // Orbit the camera around the origin. Deriving position and orientation
    // from Frame::total keeps the motion deterministic across resizes and
    // pauses.
    const float t = p_frame.total * 0.20f;
    const float radius = 80.0f;
    const float height = 30.0f + (12.0f * std::sin(t * 0.5f));
    const Vector3f cam_pos(radius * std::sin(t), height,
                           radius * std::cos(t));

    // Camera orientation as yaw * pitch. Yaw is the angle around Y that
    // aligns the local -Z with the horizontal projection of (target - pos);
    // pitch is the negative angle around X that tilts the nose down to
    // reach the origin at the given radius and height. The two rotations
    // compose cleanly as quaternions and never need a lookAt matrix.
    const float yaw = t;
    const float pitch = -std::atan2(height, radius);
    const Quatf q_yaw = Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(yaw)),
        Vector3f(0.0f, 1.0f, 0.0f));
    const Quatf q_pitch = Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(pitch)),
        Vector3f(1.0f, 0.0f, 0.0f));

    m_world.transform(m_camera).position = cam_pos;
    m_world.transform(m_camera).rotation = q_yaw * q_pitch;

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
