//==============================================================================
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
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//==============================================================================

#include "main.hpp"

#include "Compages/Physics/PhysicsWorld.hpp"
#include "Compages/World/Components/BoxCollider.hpp"
#include "Compages/World/Components/RigidBody.hpp"
#include "Compages/World/EventQueue.hpp"
#include "Compages/World/World.hpp"

#include <algorithm>
#include <limits>

//------------------------------------------------------------------------------
TEST(PhysicsWorld, DynamicCubeRestsOnFloor)
{
    world::World world;
    physics::PhysicsWorld physics;

    world::Entity floor = world.create("Floor");
    world.transform(floor).position = Vector3f(0.0f, 0.0f, 0.0f);
    world.transform(floor).scale = Vector3f(10.0f, 1.0f, 10.0f);
    world.add(floor, world::RigidBody{ world::BodyType::Static });
    world.add(floor, world::BoxCollider{ Vector3f(0.5f, 0.5f, 0.5f) });

    world::Entity cube = world.create("Cube");
    world.transform(cube).position = Vector3f(0.0f, 5.0f, 0.0f);
    world.add(cube, world::RigidBody{});
    world.add(cube, world::BoxCollider{ Vector3f(0.5f, 0.5f, 0.5f) });
    world.update();

    world::EventQueue events;
    for (int i = 0; i < 120; ++i)
    {
        physics.step(world, 1.0f / 60.0f, &events);
    }

    ASSERT_GT(world.transform(cube).position.y, 0.5f);
    ASSERT_LT(world.transform(cube).position.y, 2.0f);
    ASSERT_FALSE(events.events().empty());
}

//------------------------------------------------------------------------------
TEST(PhysicsWorld, OverlappingDynamicsSeparate)
{
    world::World world;
    physics::PhysicsWorld physics;

    world::Entity left = world.create("Left");
    world.transform(left).position = Vector3f(-0.2f, 2.0f, 0.0f);
    world.add(left, world::RigidBody{});
    world.add(left, world::BoxCollider{ Vector3f(0.5f, 0.5f, 0.5f) });

    world::Entity right = world.create("Right");
    world.transform(right).position = Vector3f(0.2f, 2.0f, 0.0f);
    world.add(right, world::RigidBody{});
    world.add(right, world::BoxCollider{ Vector3f(0.5f, 0.5f, 0.5f) });
    world.update();

    for (int i = 0; i < 60; ++i)
    {
        physics.step(world, 1.0f / 60.0f, nullptr);
    }

    const float gap =
        world.transform(right).position.x - world.transform(left).position.x;
    ASSERT_GE(gap, 0.989f);
}

//------------------------------------------------------------------------------
TEST(PhysicsWorld, StackedCubesDoNotNest)
{
    world::World world;
    physics::PhysicsWorld physics;

    world::Entity floor = world.create("Floor");
    world.transform(floor).position = Vector3f(0.0f, 0.0f, 0.0f);
    world.transform(floor).scale = Vector3f(10.0f, 1.0f, 10.0f);
    world.add(floor, world::RigidBody{ world::BodyType::Static });
    world.add(floor, world::BoxCollider{ Vector3f(0.5f, 0.5f, 0.5f) });

    world::Entity bottom = world.create("Bottom");
    world.transform(bottom).position = Vector3f(0.0f, 1.2f, 0.0f);
    world.add(bottom, world::RigidBody{});
    world.add(bottom, world::BoxCollider{ Vector3f(0.5f, 0.5f, 0.5f) });

    world::Entity top = world.create("Top");
    world.transform(top).position = Vector3f(0.0f, 2.4f, 0.0f);
    world.add(top, world::RigidBody{});
    world.add(top, world::BoxCollider{ Vector3f(0.5f, 0.5f, 0.5f) });
    world.update();

    for (int i = 0; i < 180; ++i)
    {
        physics.step(world, 1.0f / 60.0f, nullptr);
    }

    const float bottom_y = world.transform(bottom).position.y;
    const float top_y = world.transform(top).position.y;
    ASSERT_GT(bottom_y, 0.85f);
    ASSERT_LT(bottom_y, 1.2f);
    ASSERT_GE(top_y - bottom_y, 0.95f);
}

//------------------------------------------------------------------------------
TEST(PhysicsWorld, SideBySideDoesNotLaunch)
{
    world::World world;
    physics::PhysicsWorld physics;
    physics.gravity = Vector3f(0.0f, 0.0f, 0.0f);

    world::Entity left = world.create("Left");
    world.transform(left).position = Vector3f(0.0f, 1.0f, 0.0f);
    world.add(left, world::RigidBody{});
    world.add(left, world::BoxCollider{ Vector3f(0.5f, 0.5f, 0.5f) });

    world::Entity right = world.create("Right");
    world.transform(right).position = Vector3f(0.05f, 1.0f, 0.0f);
    world.add(right, world::RigidBody{});
    world.add(right, world::BoxCollider{ Vector3f(0.5f, 0.5f, 0.5f) });
    world.update();

    for (int i = 0; i < 60; ++i)
    {
        physics.step(world, 1.0f / 60.0f, nullptr);
    }

    ASSERT_NEAR(world.transform(left).position.y, 1.0f, 0.05f);
    ASSERT_NEAR(world.transform(right).position.y, 1.0f, 0.05f);
    ASSERT_GE(world.transform(right).position.x -
                  world.transform(left).position.x,
              0.989f);
}

//------------------------------------------------------------------------------
TEST(PhysicsWorld, TriggerEmitsAnEventWithoutPushingTheBody)
{
    world::World world;
    physics::PhysicsWorld physics;
    physics.gravity = Vector3f(0.0f, 0.0f, 0.0f);

    world::Entity zone = world.create("Zone");
    world.add(zone, world::RigidBody{ world::BodyType::Static });
    world.add(zone, world::BoxCollider{
                        Vector3f(1.0f, 1.0f, 1.0f), true });

    world::Entity body = world.create("Body");
    world.transform(body).position = Vector3f(0.5f, 0.0f, 0.0f);
    world.add(body, world::RigidBody{});
    world.add(body, world::BoxCollider{});
    world.update();

    world::EventQueue events;
    physics.step(world, 1.0f / 60.0f, &events);

    ASSERT_NEAR(world.transform(body).position.x, 0.5f, 0.01f);
    ASSERT_TRUE(std::ranges::any_of(
        events.events(),
        [](world::Event const& event) {
            return event.kind == world::EventKind::Trigger;
        }));
}

//------------------------------------------------------------------------------
TEST(PhysicsWorld, RaycastReturnsClosestHit)
{
    world::World world;
    physics::PhysicsWorld physics;

    world::Entity near = world.create("Near");
    world.transform(near).position = Vector3f(2.0f, 0.0f, 0.0f);
    world.add(near, world::RigidBody{ world::BodyType::Static });
    world.add(near, world::BoxCollider{});

    world::Entity far = world.create("Far");
    world.transform(far).position = Vector3f(5.0f, 0.0f, 0.0f);
    world.add(far, world::RigidBody{ world::BodyType::Static });
    world.add(far, world::BoxCollider{});

    physics.step(world, 0.0f, nullptr);
    const auto hit = physics.raycast(
        Vector3f(0.0f, 0.0f, 0.0f),
        Vector3f(4.0f, 0.0f, 0.0f),
        10.0f);

    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->entity, near);
    EXPECT_NEAR(hit->point.x, 1.5f, 1.0e-4f);
    EXPECT_NEAR(hit->normal.x, -1.0f, 1.0e-4f);
    EXPECT_NEAR(hit->fraction, 0.15f, 1.0e-4f);
    EXPECT_NEAR(hit->distance, 1.5f, 1.0e-4f);
}

//------------------------------------------------------------------------------
TEST(PhysicsWorld, RaycastReturnsNoHitForMissAndInvalidInput)
{
    world::World world;
    physics::PhysicsWorld physics;

    world::Entity box = world.create("Box");
    world.transform(box).position = Vector3f(2.0f, 0.0f, 0.0f);
    world.add(box, world::RigidBody{ world::BodyType::Static });
    world.add(box, world::BoxCollider{});
    physics.step(world, 0.0f, nullptr);

    EXPECT_FALSE(physics.raycast(
        Vector3f(0.0f, 2.0f, 0.0f),
        Vector3f(1.0f, 0.0f, 0.0f),
        10.0f));
    EXPECT_FALSE(physics.raycast({}, {}, 10.0f));
    EXPECT_FALSE(physics.raycast(
        {}, Vector3f(1.0f, 0.0f, 0.0f), 0.0f));
    EXPECT_FALSE(physics.raycast(
        {},
        Vector3f(std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f),
        10.0f));
    EXPECT_FALSE(physics.raycast(
        {},
        Vector3f(1.0f, 0.0f, 0.0f),
        std::numeric_limits<float>::infinity()));
}

//------------------------------------------------------------------------------
TEST(PhysicsWorld, RaycastReflectsDestructionAfterStep)
{
    world::World world;
    physics::PhysicsWorld physics;

    world::Entity near = world.create("Near");
    world.transform(near).position = Vector3f(2.0f, 0.0f, 0.0f);
    world.add(near, world::RigidBody{ world::BodyType::Static });
    world.add(near, world::BoxCollider{});

    world::Entity far = world.create("Far");
    world.transform(far).position = Vector3f(5.0f, 0.0f, 0.0f);
    world.add(far, world::RigidBody{ world::BodyType::Static });
    world.add(far, world::BoxCollider{});

    physics.step(world, 0.0f, nullptr);
    world.destroy(near);
    physics.step(world, 0.0f, nullptr);

    const auto hit = physics.raycast(
        Vector3f(0.0f, 0.0f, 0.0f),
        Vector3f(1.0f, 0.0f, 0.0f),
        10.0f);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->entity, far);
    EXPECT_NEAR(hit->distance, 4.5f, 1.0e-4f);
}
