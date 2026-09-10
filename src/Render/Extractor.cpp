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

#include "Render/Extractor.hpp"

#include "Assets/AssetManager.hpp"
#include "Math/Transformation.hpp"
#include "Math/Units.hpp"
#include "Scene/Scene.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/Light.hpp"
#include "World/Components/MeshRenderer.hpp"
#include "World/World.hpp"

#include <cmath>

using namespace units::literals;

namespace render
{

namespace
{

//------------------------------------------------------------------------------
CameraFrame buildCameraFrame(world::Camera const& p_camera,
                             Matrix44f const& p_world_matrix,
                             float p_aspect)
{
    CameraFrame frame;
    frame.view = matrix::inverse(p_world_matrix);
    frame.position = Vector3f(p_world_matrix[3].x,
                              p_world_matrix[3].y,
                              p_world_matrix[3].z);

    const float aspect = (p_aspect <= 0.0f) ? 1.0f : p_aspect;
    if (p_camera.projection == world::Projection::Perspective)
    {
        frame.projection = matrix::perspective(
            units::angle::degree_t(static_cast<double>(p_camera.fov_degrees)),
            aspect,
            p_camera.near_plane,
            p_camera.far_plane);
    }
    else
    {
        const float half_h = p_camera.ortho_half_height;
        const float half_w = half_h * aspect;
        frame.projection = matrix::ortho(-half_w, half_w, -half_h, half_h,
                                         p_camera.near_plane,
                                         p_camera.far_plane);
    }
    // This library composes matrices in the row-vector convention: a point
    // p is a 1x4 row and the multiplication is p * M, so applying view then
    // projection to a point p is p * view * projection. The Frustum's
    // Gribb-Hartmann extraction reads the shader-row 3 +/- shader-row i of
    // the same combined matrix. That is why the composition here is view *
    // projection, not projection * view.
    frame.view_projection = frame.view * frame.projection;
    frame.inverse_view = p_world_matrix;
    frame.inverse_projection = matrix::inverse(frame.projection);
    frame.frustum = Frustum::fromViewProjection(frame.view_projection);
    return frame;
}

//------------------------------------------------------------------------------
Vector3f forwardOf(Matrix44f const& p_world)
{
    // Rows of the CPU matrix are the columns of the shader; the local -Z
    // axis, in world space, is the "look" direction of a right-handed camera
    // and the direction a directional light points from.
    const Vector4f axis = p_world[2];
    return Vector3f(-axis.x, -axis.y, -axis.z);
}

} // namespace

//------------------------------------------------------------------------------
gloop::Result<RenderSnapshot> Extractor::extract(scene::Scene const& p_scene,
                                               float p_aspect)
{
    world::World const& world = p_scene.world();
    assets::AssetManager const& assets = p_scene.assets();

    const world::Entity camera_entity = p_scene.activeCamera();
    if (!camera_entity.valid() || !world.alive(camera_entity))
    {
        return gloop::failure(
            "no camera to extract from: the Scene's active camera Entity is "
            "empty or already destroyed");
    }
    world::Camera const* camera = world.tryGet<world::Camera>(camera_entity);
    if (camera == nullptr)
    {
        return gloop::failure(
            "the Scene's active camera Entity does not carry a Camera "
            "component");
    }

    RenderSnapshot snapshot;
    snapshot.environment = p_scene.environment();
    snapshot.camera = buildCameraFrame(*camera,
                                       world.worldMatrix(camera_entity),
                                       p_aspect);

    const bool cull = p_scene.renderSettings().frustum_culling;

    // Meshes.
    auto const& renderer_store = world.components<world::MeshRenderer>();
    auto const renderers = renderer_store.components();
    auto const renderer_entities = renderer_store.entities();
    snapshot.items.reserve(renderers.size());
    for (std::size_t i = 0u; i < renderers.size(); ++i)
    {
        const world::Entity entity = renderer_entities[i];
        world::MeshRenderer const& mr = renderers[i];
        if (!world.alive(entity))
        {
            continue;
        }
        if (!world.enabledInHierarchy(entity))
        {
            continue;
        }

        assets::MeshAsset const* mesh_asset = assets.mesh(mr.mesh);
        if (mesh_asset == nullptr)
        {
            continue;
        }
        assets::MaterialInstance const* instance =
            assets.materialInstance(mr.material_instance);
        if (!mr.material_instance.valid() || (instance == nullptr) ||
            (assets.material(instance->material) == nullptr))
        {
            continue;
        }

        RenderItem item;
        item.entity = entity;
        item.mesh = mr.mesh;
        item.material_instance = mr.material_instance;
        item.world_matrix = world.worldMatrix(entity);
        item.world_bounds = mesh_asset->local_bounds.transformed(item.world_matrix);
        item.flags = mr.flags;

        if (cull && !snapshot.camera.frustum.contains(item.world_bounds))
        {
            continue;
        }

        snapshot.items.push_back(item);
    }

    // Directional lights.
    auto const& dir_store = world.components<world::DirectionalLight>();
    auto const dir_lights = dir_store.components();
    auto const dir_entities = dir_store.entities();
    for (std::size_t i = 0u; i < dir_lights.size(); ++i)
    {
        const world::Entity entity = dir_entities[i];
        if (!world.alive(entity) || !world.enabledInHierarchy(entity))
        {
            continue;
        }
        DirectionalLightFrame frame;
        frame.direction = forwardOf(world.worldMatrix(entity));
        const float m = std::abs(frame.direction.x) +
                        std::abs(frame.direction.y) +
                        std::abs(frame.direction.z);
        if (m < 1.0e-6f)
        {
            frame.direction = p_scene.environment().default_light_direction;
        }
        frame.color = dir_lights[i].color;
        frame.intensity = dir_lights[i].intensity;
        snapshot.directional_lights.push_back(frame);
    }

    // Point lights.
    auto const& point_store = world.components<world::PointLight>();
    auto const point_lights = point_store.components();
    auto const point_entities = point_store.entities();
    for (std::size_t i = 0u; i < point_lights.size(); ++i)
    {
        const world::Entity entity = point_entities[i];
        if (!world.alive(entity) || !world.enabledInHierarchy(entity))
        {
            continue;
        }
        PointLightFrame frame;
        const Matrix44f& world_matrix = world.worldMatrix(entity);
        frame.position = Vector3f(world_matrix[3].x,
                                  world_matrix[3].y,
                                  world_matrix[3].z);
        frame.color = point_lights[i].color;
        frame.intensity = point_lights[i].intensity;
        frame.range = point_lights[i].range;
        snapshot.point_lights.push_back(frame);
    }

    return snapshot;
}

//------------------------------------------------------------------------------
gloop::Result<RenderSnapshot> Extractor::extract(scene::Scene const& p_scene,
                                               std::uint32_t p_width,
                                               std::uint32_t p_height)
{
    const float aspect =
        (p_height == 0u)
            ? 1.0f
            : (static_cast<float>(p_width) / static_cast<float>(p_height));
    return extract(p_scene, aspect);
}

} // namespace render
