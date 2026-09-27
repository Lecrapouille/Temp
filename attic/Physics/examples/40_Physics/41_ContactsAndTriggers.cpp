//=============================================================================
// Compages: A C++20 GPU, rendering and simulation library.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//=============================================================================

#include "40_Physics/41_ContactsAndTriggers.hpp"

#include "Compages/World/Components/BoxCollider.hpp"
#include "Compages/World/Components/RigidBody.hpp"

namespace examples
{

namespace
{

bool samePair(world::Event const& p_event,
              world::Entity p_first,
              world::Entity p_second)
{
    return ((p_event.a == p_first) && (p_event.b == p_second)) ||
           ((p_event.a == p_second) && (p_event.b == p_first));
}

} // namespace

std::string ContactsAndTriggers::description() const
{
    return "Headless EventQueue validation: one overlapping solid pair emits "
           "Collision while a separate trigger pair emits Trigger without "
           "backend-specific access or GPU resources.";
}

gpu::Status ContactsAndTriggers::setUp()
{
    m_physics.gravity = {};

    world::Entity wall = m_world.create("Solid wall");
    m_world.transform(wall).position = Vector3f(-3.0f, 0.0f, 0.0f);
    m_world.add(wall, world::RigidBody{ world::BodyType::Static });
    m_world.add(wall, world::BoxCollider{});

    world::Entity solid = m_world.create("Solid body");
    m_world.transform(solid).position = Vector3f(-2.7f, 0.0f, 0.0f);
    m_world.add(solid, world::RigidBody{});
    m_world.add(solid, world::BoxCollider{});

    world::Entity zone = m_world.create("Trigger zone");
    m_world.transform(zone).position = Vector3f(3.0f, 0.0f, 0.0f);
    m_world.add(zone, world::RigidBody{ world::BodyType::Static });
    m_world.add(zone, world::BoxCollider{ Vector3f(0.75f, 0.75f, 0.75f),
                                          true });

    world::Entity visitor = m_world.create("Trigger visitor");
    m_world.transform(visitor).position = Vector3f(3.2f, 0.0f, 0.0f);
    m_world.add(visitor, world::RigidBody{});
    m_world.add(visitor, world::BoxCollider{});

    m_events.clear();
    m_physics.step(m_world, 1.0f / 60.0f, &m_events);

    bool collision = false;
    bool trigger = false;
    for (world::Event const& event : m_events.events())
    {
        collision = collision ||
                    ((event.kind == world::EventKind::Collision) &&
                     samePair(event, wall, solid));
        trigger = trigger ||
                  ((event.kind == world::EventKind::Trigger) &&
                   samePair(event, zone, visitor));
    }
    m_validated = collision && trigger;
    return m_validated
               ? gpu::success()
               : gpu::failure("collision and trigger events were not distinct");
}

void ContactsAndTriggers::draw(Frame const& )
{
    gpu::check(m_validated
               ? gpu::success()
               : gpu::failure("contact/trigger validation did not run"));
}

} // namespace examples
