//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#pragma once

#include "Compages/Math/Vector.hpp"
#include "Compages/World/Entity.hpp"

#include <memory>
#include <optional>

namespace world
{
class EventQueue;
class World;
}

namespace physics
{

// ****************************************************************************
//! \brief Backend-independent result of a world-space physics ray query.
// ****************************************************************************
struct RaycastHit
{
    world::Entity entity;
    Vector3f point;
    Vector3f normal;
    //! Fraction along the finite segment, in [0, 1].
    float fraction = 0.0f;
    //! Distance from the ray origin in world units.
    float distance = 0.0f;
};

// ****************************************************************************
//! \brief Rigid-body simulation backed by ReactPhysics3D behind a PImpl.
//!
//! Collider and RigidBody components are synchronized into the backend by
//! \c step(), which advances dynamic bodies and writes their poses back into
//! LocalTransform. Queries operate on that last synchronized backend state.
// ****************************************************************************
class PhysicsWorld
{
public:

    PhysicsWorld();
    ~PhysicsWorld();
    PhysicsWorld(PhysicsWorld const&) = delete;
    PhysicsWorld& operator=(PhysicsWorld const&) = delete;
    PhysicsWorld(PhysicsWorld&&) noexcept;
    PhysicsWorld& operator=(PhysicsWorld&&) noexcept;

    Vector3f gravity{ 0.0f, -9.81f, 0.0f };

    void step(world::World& p_world, float p_dt, world::EventQueue* p_events);

    // ------------------------------------------------------------------------
    //! \brief Return the closest collider hit by a finite world-space ray.
    //!
    //! Call \c step() after adding, moving or destroying colliders before
    //! querying. Direction is normalized internally. A zero/non-finite
    //! direction or a non-positive/non-finite distance returns no hit.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::optional<RaycastHit>
    raycast(Vector3f p_origin,
            Vector3f p_direction,
            float p_max_distance) const;

private:

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace physics
