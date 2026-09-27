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

#include "Compages/Scene/FPSController.hpp"
#include "Compages/Scene/FlyController.hpp"
#include "Compages/Scene/OrbitController.hpp"
#include "Compages/Scene/ThirdPersonController.hpp"
#include "Compages/Scene/World.hpp"

//------------------------------------------------------------------------------
TEST(OrbitController, PlacesTheCameraOppositeTheForwardAxis)
{
    scene::World world;
    const scene::EntityId camera = world.create("cam");
    scene::OrbitController orbit;
    orbit.target = Vector3f(0.0f, 0.0f, 0.0f);
    orbit.yaw = 0.0f;
    orbit.pitch = 0.0f;
    orbit.distance = 10.0f;
    orbit.writePose(world, camera);

    // Yaw 0, pitch 0: looking along -Z, so the camera sits at +Z.
    ASSERT_NEAR(world.transform(camera).position.x, 0.0f, 1.0e-4f);
    ASSERT_NEAR(world.transform(camera).position.y, 0.0f, 1.0e-4f);
    ASSERT_NEAR(world.transform(camera).position.z, 10.0f, 1.0e-4f);
}

//------------------------------------------------------------------------------
TEST(OrbitController, ZoomPullsTheCameraIn)
{
    scene::World world;
    const scene::EntityId camera = world.create("cam");
    scene::OrbitController orbit;
    orbit.distance = 20.0f;
    scene::CameraInput input;
    input.zoom_delta = 2.0f;
    orbit.apply(world, camera, input);
    ASSERT_NEAR(orbit.distance, 20.0f - (2.0f * orbit.zoom_sensitivity),
                1.0e-4f);
}

//------------------------------------------------------------------------------
TEST(FlyController, ForwardMovesAlongLook)
{
    scene::World world;
    const scene::EntityId camera = world.create("cam");
    world.transform(camera).position = Vector3f(0.0f, 0.0f, 0.0f);
    scene::FlyController fly;
    fly.yaw = 0.0f;
    fly.pitch = 0.0f;
    fly.move_speed = 10.0f;
    scene::CameraInput input;
    input.forward = true;
    fly.apply(world, camera, input, 1.0f);
    // Looking -Z, one second at 10 units/s.
    ASSERT_NEAR(world.transform(camera).position.z, -10.0f, 1.0e-3f);
}

//------------------------------------------------------------------------------
TEST(FPSController, WalksOnTheGroundWithoutClimbing)
{
    scene::World world;
    const scene::EntityId camera = world.create("cam");
    world.transform(camera).position = Vector3f(0.0f, 1.7f, 0.0f);
    scene::FPSController fps;
    fps.yaw = 0.0f;
    fps.pitch = -0.4f;
    fps.eye_height = 1.7f;
    fps.move_speed = 5.0f;
    scene::CameraInput input;
    input.forward = true;
    fps.apply(world, camera, input, 1.0f);
    ASSERT_NEAR(world.transform(camera).position.y, 1.7f, 1.0e-4f);
    ASSERT_NEAR(world.transform(camera).position.z, -5.0f, 1.0e-3f);
}

//------------------------------------------------------------------------------
TEST(ThirdPersonController, FollowsTheTargetEntity)
{
    scene::World world;
    const scene::EntityId target = world.create("hero");
    const scene::EntityId camera = world.create("cam");
    world.transform(target).position = Vector3f(10.0f, 0.0f, 0.0f);
    scene::ThirdPersonController follow;
    follow.target = target;
    follow.look_offset = Vector3f(0.0f, 0.0f, 0.0f);
    follow.yaw = 0.0f;
    follow.pitch = 0.0f;
    follow.distance = 6.0f;
    follow.writePose(world, camera);

    ASSERT_NEAR(world.transform(camera).position.x, 10.0f, 1.0e-4f);
    ASSERT_NEAR(world.transform(camera).position.z, 6.0f, 1.0e-4f);
}
