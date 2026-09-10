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

#include "Render/Picker.hpp"

#include "Assets/AssetManager.hpp"
#include "Render/CameraFrame.hpp"
#include "Scene/Scene.hpp"
#include "World/World.hpp"

namespace render
{

//------------------------------------------------------------------------------
std::optional<world::RayHit> pick(scene::Scene const& p_scene, Ray const& p_ray)
{
    world::World const& world = p_scene.world();
    assets::AssetManager const& assets = p_scene.assets();
    return world::raycast(
        world,
        p_ray,
        [&](world::Entity p_entity, world::MeshRenderer const& p_renderer)
        {
            assets::MeshAsset const* mesh = assets.mesh(p_renderer.mesh);
            if (mesh == nullptr)
            {
                return AABB{};
            }
            return mesh->local_bounds.transformed(world.worldMatrix(p_entity));
        });
}

//------------------------------------------------------------------------------
std::optional<world::RayHit> pickAt(scene::Scene const& p_scene,
                                    CameraFrame const& p_camera,
                                    float p_x,
                                    float p_y,
                                    std::uint32_t p_width,
                                    std::uint32_t p_height)
{
    return pick(p_scene, p_camera.screenRay(p_x, p_y, p_width, p_height));
}

} // namespace render
