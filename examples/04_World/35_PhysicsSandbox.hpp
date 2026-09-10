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

#pragma once

#include "Common/Example.hpp"

#include "Assets/AssetIds.hpp"
#include "Assets/AssetManager.hpp"
#include "Math/Quaternion.hpp"
#include "Math/Vector.hpp"
#include "Physics/PhysicsWorld.hpp"
#include "Render/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "World/Entity.hpp"
#include "World/EventQueue.hpp"
#include "World/World.hpp"

#include <string_view>
#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief AABB physics as its own demo, not buried in the MVP kitchen sink.
//!
//! Dynamic cubes, spheres, cones and cylinders fall under gravity, rest on a
//! static floor, and get shoved by a kinematic pusher. Colliders stay AABB
//! (the visual mesh is only what is drawn). Bodies that leave the floor are
//! dropped back in from above so --check always has motion.
// ****************************************************************************
class PhysicsSandbox: public Example
{
public:

    PhysicsSandbox() : m_scene(m_world, m_assets) {}

    [[nodiscard]] std::string name() const override
    {
        return "35_PhysicsSandbox";
    }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    struct PropKind
    {
        std::string_view name;
        assets::MeshAssetId mesh;
        assets::MaterialInstanceId material;
        Vector3f half_extent{ 0.5f, 0.5f, 0.5f };
        Quatf rotation{};
    };

    [[nodiscard]] gpu::Status spawnBody(Vector3f const& p_place,
                                        PropKind const& p_kind);

    assets::AssetManager m_assets;
    world::World m_world;
    scene::Scene m_scene;
    render::Renderer m_renderer;
    physics::PhysicsWorld m_physics;
    world::EventQueue m_events;

    assets::MeshAssetId m_cube_mesh;
    assets::MaterialInstanceId m_floor_mat;
    assets::MaterialInstanceId m_hit_mat;
    assets::MaterialInstanceId m_pusher_mat;

    world::Entity m_camera;
    world::Entity m_sun;
    world::Entity m_floor;
    world::Entity m_pusher;
    std::vector<world::Entity> m_bodies;
    std::vector<assets::MaterialInstanceId> m_idle_mats;
};

} // namespace examples
