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

#include "04_World/36_GltfAnimation.hpp"

#include "Assets/Loaders/GltfLoader.hpp"
#include "Common/DataPath.hpp"
#include "Common/File.hpp"
#include "GPU/RenderPass.hpp"
#include "Math/AABB.hpp"
#include "Math/Units.hpp"
#include "Render/Extractor.hpp"
#include "World/AnimationSystem.hpp"
#include "World/CameraInput.hpp"
#include "World/Components/Animator.hpp"
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

[[nodiscard]] std::string soldierPath()
{
    const std::string bundled = dataPath("Soldier.glb");
    if (!bundled.empty())
    {
        return bundled;
    }
    constexpr char const* nearby[] = {
        "/home/qq/three.js/examples/models/gltf/Soldier.glb",
        "external/three.js/examples/models/gltf/Soldier.glb",
    };
    for (char const* path : nearby)
    {
        if (File::exist(path))
        {
            return path;
        }
    }
    return {};
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

[[nodiscard]] assets::AnimationClipId
clipNamed(assets::AssetManager const& p_assets,
          std::vector<assets::AnimationClipId> const& p_clips,
          char const* p_name)
{
    for (assets::AnimationClipId const id : p_clips)
    {
        assets::AnimationClip const* clip = p_assets.animation(id);
        if ((clip != nullptr) && (clip->name == p_name))
        {
            return id;
        }
    }
    return {};
}

} // namespace

//------------------------------------------------------------------------------
std::string GltfAnimation::description() const
{
    return "importGltf reads Soldier.glb skins and clips. AnimationSystem "
           "samples Walk onto the Mixamo joints and skins the mesh each "
           "frame. Keys 1 / 2 / 3 switch Idle / Walk / Run. Right-drag orbits.";
}

//------------------------------------------------------------------------------
gpu::Status GltfAnimation::setUp()
{
    const std::string path = soldierPath();
    if (path.empty())
    {
        return gpu::failure(
            "36_GltfAnimation needs Soldier.glb (OpenGLCppWrapper-data or "
            "three.js examples/models/gltf/).");
    }

    GPU_TRY_ASSIGN(imported, assets::importGltf(path, m_assets, m_world));
    if (imported.mesh_count == 0u)
    {
        return gpu::failure("importGltf loaded '" + path +
                            "' but found no mesh primitives");
    }
    if (imported.animations.empty())
    {
        return gpu::failure("'" + path + "' has no animation clips");
    }
    m_clips = imported.animations;
    m_animator = imported.animator;

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

    m_sun = m_world.create("Sun");
    m_world.transform(m_sun).rotation =
        Quatf::fromAngleAxis(units::angle::radian_t(-0.75),
                             Vector3f(1.0f, 0.0f, 0.0f));
    m_world.add(m_sun,
                world::DirectionalLight{ Vector3f(1.0f, 0.97f, 0.90f), 1.15f });

    m_camera = m_world.create("Camera");
    world::Camera camera;
    camera.projection = world::Projection::Perspective;
    camera.fov_degrees = 40.0f;
    camera.near_plane = std::max(radius * 0.01f, 0.05f);
    camera.far_plane = std::max(radius * 40.0f, 20.0f);
    m_world.add(m_camera, camera);

    m_orbit.target = center + Vector3f(0.0f, extent.y * 0.15f, 0.0f);
    m_orbit.distance = radius * 3.4f;
    m_orbit.min_distance = std::max(radius * 0.4f, 0.2f);
    m_orbit.max_distance = radius * 16.0f;
    m_orbit.pitch = -0.15f;
    m_orbit.yaw = 0.55f;
    m_orbit.writePose(m_world, m_camera);

    m_scene.setActiveCamera(m_camera);
    m_scene.renderSettings().clear_color = Vector4f(0.10f, 0.12f, 0.15f, 1.0f);
    m_scene.environment().ambient = Vector3f(0.22f, 0.22f, 0.24f);

    GPU_TRY(world::AnimationSystem::tick(m_world, m_assets, 0.0f));
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status GltfAnimation::draw(Frame const& p_frame)
{
    if (m_world.alive(m_animator) && m_world.has<world::Animator>(m_animator))
    {
        world::Animator& animator = m_world.get<world::Animator>(m_animator);
        auto choose = [&](char const* p_name) {
            const assets::AnimationClipId id =
                clipNamed(m_assets, m_clips, p_name);
            if (id.valid() && (animator.clip != id))
            {
                animator.clip = id;
                animator.time = 0.0f;
                animator.playing = true;
            }
        };
        if (p_frame.key_1)
        {
            choose("Idle");
        }
        else if (p_frame.key_2)
        {
            choose("Walk");
        }
        else if (p_frame.key_3)
        {
            choose("Run");
        }
    }

    GPU_TRY(world::AnimationSystem::tick(m_world, m_assets, p_frame.elapsed));

    m_orbit.apply(m_world, m_camera, inputFrom(p_frame));
    m_world.update();

    GPU_TRY_ASSIGN(
        snapshot,
        render::Extractor::extract(m_scene, p_frame.width, p_frame.height));
    if (snapshot.items.empty())
    {
        return gpu::failure("36_GltfAnimation extracted no drawable mesh");
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
