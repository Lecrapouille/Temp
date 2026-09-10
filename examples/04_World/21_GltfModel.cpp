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

#include "04_World/21_GltfModel.hpp"

#include "Assets/Loaders/GltfLoader.hpp"
#include "Common/DataPath.hpp"
#include "GPU/RenderPass.hpp"
#include "Math/AABB.hpp"
#include "Math/Units.hpp"
#include "Render/Extractor.hpp"
#include "World/CameraInput.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/Light.hpp"
#include "World/Components/MeshRenderer.hpp"

#include <algorithm>
#include <cmath>

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
    return input;
}

[[nodiscard]] AABB meshBounds(world::World const& p_world,
                              assets::AssetManager const& p_assets)
{
    AABB bounds;
    auto const& store = p_world.components<world::MeshRenderer>();
    auto const meshes = store.components();
    auto const entities = store.entities();
    for (std::size_t i = 0u; i < meshes.size(); ++i)
    {
        assets::MeshAsset const* mesh = p_assets.mesh(meshes[i].mesh);
        if (mesh == nullptr)
        {
            continue;
        }
        bounds = bounds.merged(
            mesh->local_bounds.transformed(p_world.worldMatrix(entities[i])));
    }
    return bounds;
}

} // namespace

//------------------------------------------------------------------------------
std::string GltfModel::description() const
{
    return "importGltf reads a GLB: meshes and embedded textures become "
           "AssetManager entries, nodes become Entities with MeshRenderer "
           "components. ShaderLib's PBR shader samples the albedo map. "
           "Right-drag orbits, scroll zooms. Needs Duck.glb in "
           "external/OpenGLCppWrapper-data/.";
}

//------------------------------------------------------------------------------
gpu::Status GltfModel::setUp()
{
    const std::string path = dataPath("Duck.glb");
    if (path.empty())
    {
        return gpu::failure(
            "21_GltfModel needs Duck.glb from OpenGLCppWrapper-data. "
            "Run make download in external/, or set GLOOP_DATA_PATH.");
    }

    GPU_TRY_ASSIGN(imported, assets::importGltf(path, m_assets, m_world));
    if (imported.mesh_count == 0u)
    {
        return gpu::failure("importGltf loaded '" + path +
                            "' but found no mesh primitives");
    }

    m_model_root = imported.root;

    const AABB bounds = meshBounds(m_world, m_assets);
    if (bounds.empty())
    {
        return gpu::failure("importGltf produced no drawable bounds for '" +
                            path + "'");
    }

    const Vector3f center = bounds.center();
    const Vector3f extent = bounds.extent();
    const float radius =
        std::max({ extent.x, extent.y, extent.z, 0.05f });

    // Keep the file's node TRS (Duck is stored in centimetres with a 0.01
    // root). Frame the camera on the world bounds instead of rewriting scale.
    m_sun = m_world.create("Sun");
    m_world.transform(m_sun).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(-0.8),
                             Vector3f(1.0f, 0.0f, 0.0f));
    m_world.add(m_sun,
                world::DirectionalLight{ Vector3f(1.0f, 0.98f, 0.92f), 1.2f });

    m_camera = m_world.create("Camera");
    world::Camera camera;
    camera.projection = world::Projection::Perspective;
    camera.fov_degrees = 45.0f;
    camera.near_plane = std::max(radius * 0.01f, 0.01f);
    camera.far_plane = std::max(radius * 40.0f, 20.0f);
    m_world.add(m_camera, camera);

    m_orbit.target = center;
    m_orbit.distance = radius * 4.0f;
    m_orbit.min_distance = std::max(radius * 0.35f, 0.15f);
    m_orbit.max_distance = radius * 20.0f;
    m_orbit.pitch = -0.25f;
    m_orbit.writePose(m_world, m_camera);

    m_scene.setActiveCamera(m_camera);
    m_scene.renderSettings().clear_color = Vector4f(0.12f, 0.14f, 0.18f, 1.0f);
    m_scene.environment().ambient = Vector3f(0.22f, 0.22f, 0.24f);

    m_world.update();
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status GltfModel::draw(Frame const& p_frame)
{
    m_orbit.yaw = p_frame.total * 0.4f;
    m_orbit.apply(m_world, m_camera, inputFrom(p_frame));
    m_world.update();

    GPU_TRY_ASSIGN(
        snapshot,
        render::Extractor::extract(m_scene, p_frame.width, p_frame.height));
    if (snapshot.items.empty())
    {
        return gpu::failure(
            "21_GltfModel extracted no drawable mesh. The GLB imported "
            "but its MaterialInstance does not resolve to a Material");
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
