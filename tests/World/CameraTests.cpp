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

#include "Assets/AssetManager.hpp"
#include "Render/Extractor.hpp"
#include "Scene/Scene.hpp"
#include "World/Components/Camera.hpp"
#include "World/World.hpp"

//------------------------------------------------------------------------------
// The camera is a component attached to an Entity. Its pose is the entity's
// transform. Extraction turns that into a CameraFrame.
//------------------------------------------------------------------------------
TEST(WorldCamera, ExtractsAViewFromTheEntityTransform)
{
    world::World world;
    assets::AssetManager assets;
    scene::Scene scene(world, assets);

    world::Entity cam = world.create("camera");
    world.transform(cam).position = Vector3f(0.0f, 0.0f, 3.0f);
    world.add(cam, world::Camera{});
    scene.setActiveCamera(cam);
    world.update();

    auto snapshot = render::Extractor::extract(scene, 1.0f);
    ASSERT_TRUE(bool(snapshot)) << snapshot.error();
    ASSERT_NEAR(snapshot.value().camera.position.z, 3.0f, 1.0e-4f);
    // The view is the inverse of the entity's world matrix, so its z
    // translation is -3.
    ASSERT_NEAR(snapshot.value().camera.view[3].z, -3.0f, 1.0e-4f);
}

//------------------------------------------------------------------------------
TEST(WorldCamera, RefusesWithoutAnActiveCamera)
{
    world::World world;
    assets::AssetManager assets;
    scene::Scene scene(world, assets);

    auto snapshot = render::Extractor::extract(scene, 1.0f);
    ASSERT_FALSE(bool(snapshot));
    ASSERT_THAT(snapshot.error(), HasSubstr("camera"));
}

//------------------------------------------------------------------------------
TEST(WorldCamera, RefusesWhenActiveEntityHasNoCameraComponent)
{
    world::World world;
    assets::AssetManager assets;
    scene::Scene scene(world, assets);

    world::Entity e = world.create("not_a_camera");
    scene.setActiveCamera(e);
    world.update();

    auto snapshot = render::Extractor::extract(scene, 1.0f);
    ASSERT_FALSE(bool(snapshot));
    ASSERT_THAT(snapshot.error(), HasSubstr("Camera"));
}

//------------------------------------------------------------------------------
TEST(WorldCamera, OrthographicProjection)
{
    world::World world;
    assets::AssetManager assets;
    scene::Scene scene(world, assets);

    world::Entity cam = world.create("cam");
    world.add(cam, world::Camera{
                       world::Projection::Orthographic, 60.0f, 5.0f, 0.1f, 100.0f });
    scene.setActiveCamera(cam);
    world.update();

    auto snapshot = render::Extractor::extract(scene, 1.0f);
    ASSERT_TRUE(bool(snapshot)) << snapshot.error();
    // Half-height is 5, aspect is 1 so half-width is 5; the projection
    // maps x=5 to clip x=1.
    ASSERT_NEAR(snapshot.value().camera.projection[0].x, 1.0f / 5.0f,
                1.0e-4f);
}
