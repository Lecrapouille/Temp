//==============================================================================
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
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//==============================================================================

#include "main.hpp"

#include "Physics/PhysicsWorld.hpp"
#include "World/Components/BoxCollider.hpp"
#include "World/Components/RigidBody.hpp"
#include "World/EventQueue.hpp"
#include "World/World.hpp"

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

    physics.step(world, 1.0f / 60.0f, nullptr);

    const float gap =
        world.transform(right).position.x - world.transform(left).position.x;
    ASSERT_GE(gap, 0.99f);
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

    physics.step(world, 1.0f / 60.0f, nullptr);

    ASSERT_NEAR(world.transform(left).position.y, 1.0f, 0.05f);
    ASSERT_NEAR(world.transform(right).position.y, 1.0f, 0.05f);
    ASSERT_GE(world.transform(right).position.x -
                  world.transform(left).position.x,
              0.99f);
}
