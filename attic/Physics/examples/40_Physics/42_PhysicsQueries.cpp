//=============================================================================
// Compages: A C++20 GPU, rendering and simulation library.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//=============================================================================

#include "40_Physics/42_PhysicsQueries.hpp"

#include "Compages/World/Components/BoxCollider.hpp"
#include "Compages/World/Components/RigidBody.hpp"

#include <cmath>

namespace examples
{

std::string PhysicsQueries::description() const
{
    return "Headless public PhysicsWorld raycast: two colliders are synchronized "
           "by step(), then a finite ray validates the closest entity, world "
           "point, normal, fraction and distance without backend access.";
}

gpu::Status PhysicsQueries::setUp()
{
    world::Entity closest = m_world.create("Closest target");
    m_world.transform(closest).position = Vector3f(2.0f, 0.0f, 0.0f);
    m_world.add(closest, world::RigidBody{ world::BodyType::Static });
    m_world.add(closest, world::BoxCollider{});

    world::Entity farther = m_world.create("Farther target");
    m_world.transform(farther).position = Vector3f(5.0f, 0.0f, 0.0f);
    m_world.add(farther, world::RigidBody{ world::BodyType::Static });
    m_world.add(farther, world::BoxCollider{});

    m_physics.step(m_world, 0.0f, nullptr);
    const auto hit = m_physics.raycast(
        Vector3f(0.0f, 0.0f, 0.0f),
        Vector3f(3.0f, 0.0f, 0.0f), 10.0f);
    m_validated =
        hit && (hit->entity == closest) &&
        (std::abs(hit->point.x - 1.5f) < 1.0e-4f) &&
        (std::abs(hit->normal.x + 1.0f) < 1.0e-4f) &&
        (std::abs(hit->fraction - 0.15f) < 1.0e-4f) &&
        (std::abs(hit->distance - 1.5f) < 1.0e-4f);
    return m_validated
               ? gpu::success()
               : gpu::failure("public raycast did not return the closest target");
}

void PhysicsQueries::draw(Frame const& )
{
    gpu::check(m_validated
               ? gpu::success()
               : gpu::failure("raycast validation did not run"));
}

} // namespace examples
