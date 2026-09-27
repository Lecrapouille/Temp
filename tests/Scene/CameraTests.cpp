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

#include "Compages/Scene/Assets/AssetManager.hpp"
#include "Compages/Scene/Render/SceneExtractor.hpp"
#include "Compages/Scene/Scene.hpp"
#include "Compages/Scene/Camera.hpp"
#include "Compages/Scene/World.hpp"

//------------------------------------------------------------------------------
// The camera is a component attached to an EntityId. Its pose is the entity's
// transform. Extraction turns that into a CameraFrame.
//------------------------------------------------------------------------------
TEST(WorldCamera, ExtractsAViewFromTheEntityTransform)
{
    scene::World world;
    scene::AssetManager assets;
    scene::Scene scene(world, assets);

    scene::EntityId cam = world.create("camera");
    world.transform(cam).position = Vector3f(0.0f, 0.0f, 3.0f);
    world.add(cam, scene::Camera{});
    scene.activeCamera(cam);
    world.update();

    auto snapshot = scene::SceneExtractor::extract(scene, 1.0f);
    ASSERT_TRUE(bool(snapshot)) << snapshot.error();
    ASSERT_NEAR(snapshot.value().camera.position.z, 3.0f, 1.0e-4f);
    // The view is the inverse of the entity's world matrix, so its z
    // translation is -3.
    ASSERT_NEAR(snapshot.value().camera.view[3].z, -3.0f, 1.0e-4f);
}

//------------------------------------------------------------------------------
TEST(WorldCamera, RefusesWithoutAnActiveCamera)
{
    scene::World world;
    scene::AssetManager assets;
    scene::Scene scene(world, assets);

    auto snapshot = scene::SceneExtractor::extract(scene, 1.0f);
    ASSERT_FALSE(bool(snapshot));
    ASSERT_THAT(snapshot.error(), HasSubstr("camera"));
}

//------------------------------------------------------------------------------
TEST(WorldCamera, RefusesWhenActiveEntityHasNoCameraComponent)
{
    scene::World world;
    scene::AssetManager assets;
    scene::Scene scene(world, assets);

    scene::EntityId e = world.create("not_a_camera");
    scene.activeCamera(e);
    world.update();

    auto snapshot = scene::SceneExtractor::extract(scene, 1.0f);
    ASSERT_FALSE(bool(snapshot));
    ASSERT_THAT(snapshot.error(), HasSubstr("Camera"));
}

//------------------------------------------------------------------------------
TEST(WorldCamera, OrthographicProjection)
{
    scene::World world;
    scene::AssetManager assets;
    scene::Scene scene(world, assets);

    scene::EntityId cam = world.create("cam");
    world.add(cam, scene::Camera{
                       scene::Projection::Orthographic, 60.0f, 5.0f, 0.1f, 100.0f });
    scene.activeCamera(cam);
    world.update();

    auto snapshot = scene::SceneExtractor::extract(scene, 1.0f);
    ASSERT_TRUE(bool(snapshot)) << snapshot.error();
    // Half-height is 5, aspect is 1 so half-width is 5; the projection
    // maps x=5 to clip x=1.
    ASSERT_NEAR(snapshot.value().camera.projection[0].x, 1.0f / 5.0f,
                1.0e-4f);
}

//------------------------------------------------------------------------------
TEST(WorldCamera, ViewportBecomesAPixelRectOnTheFrame)
{
    scene::World world;
    scene::AssetManager assets;
    scene::Scene scene(world, assets);

    scene::EntityId cam = world.create("cam");
    scene::Camera camera;
    camera.viewport = { 0.25f, 0.0f, 0.5f, 1.0f };
    world.add(cam, camera);
    scene.activeCamera(cam);
    world.update();

    auto snapshot = scene::SceneExtractor::extract(scene, 800u, 400u);
    ASSERT_TRUE(bool(snapshot)) << snapshot.error();
    ASSERT_NEAR(snapshot.value().camera.viewport_x, 200.0f, 0.6f);
    ASSERT_EQ(snapshot.value().camera.viewport_width, 400u);
    ASSERT_EQ(snapshot.value().camera.viewport_height, 400u);
    // Half the framebuffer width, same height → aspect 1, same as a square.
    ASSERT_NEAR(snapshot.value().camera.projection[1].y,
                snapshot.value().camera.projection[0].x,
                1.0e-4f);
}
