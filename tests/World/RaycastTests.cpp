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
#include "World/Components/MeshRenderer.hpp"
#include "World/Raycast.hpp"
#include "World/World.hpp"

//------------------------------------------------------------------------------
TEST(Raycast, PicksTheCloserOfTwoBoxes)
{
    world::World world;
    const world::Entity near = world.create("near");
    const world::Entity far = world.create("far");
    world.add(near, world::MeshRenderer{});
    world.add(far, world::MeshRenderer{});
    world.update();

    const AABB near_box =
        AABB::fromCenterExtent(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(1.0f));
    const AABB far_box =
        AABB::fromCenterExtent(Vector3f(0.0f, 0.0f, -8.0f), Vector3f(1.0f));

    const Ray ray = Ray::fromPoints(Vector3f(0.0f, 0.0f, 10.0f),
                                    Vector3f(0.0f, 0.0f, 0.0f));
    const auto hit = world::raycast(
        world,
        ray,
        [&](world::Entity p_entity, world::MeshRenderer const&)
        {
            return (p_entity == near) ? near_box : far_box;
        });
    ASSERT_TRUE(hit.has_value());
    ASSERT_TRUE(hit->entity == near);
}

//------------------------------------------------------------------------------
TEST(Raycast, SkipsADisabledEntity)
{
    world::World world;
    const world::Entity hidden = world.create("hidden");
    world.add(hidden, world::MeshRenderer{});
    world.setEnabled(hidden, false);
    world.update();

    const Ray ray = Ray::fromPoints(Vector3f(0.0f, 0.0f, 10.0f),
                                    Vector3f(0.0f, 0.0f, 0.0f));
    const auto hit = world::raycast(
        world,
        ray,
        [&](world::Entity, world::MeshRenderer const&)
        {
            return AABB::fromCenterExtent(Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(1.0f));
        });
    ASSERT_FALSE(hit.has_value());
}

//------------------------------------------------------------------------------
TEST(WorldCamera, ScreenRayThroughTheCentreHitsTheOrigin)
{
    world::World world;
    assets::AssetManager assets;
    scene::Scene scene(world, assets);

    world::Entity cam = world.create("cam");
    world.transform(cam).position = Vector3f(0.0f, 0.0f, 10.0f);
    world.add(cam, world::Camera{});
    scene.setActiveCamera(cam);
    world.update();

    auto snapshot = render::Extractor::extract(scene, 1.0f);
    ASSERT_TRUE(bool(snapshot)) << snapshot.error();

    const Ray ray = snapshot.value().camera.screenRay(400.0f, 300.0f, 800u, 600u);
    // Centre of the view, looking along -Z: the ray should pass near the
    // origin and travel toward decreasing Z.
    ASSERT_NEAR(ray.direction.x, 0.0f, 0.05f);
    ASSERT_NEAR(ray.direction.y, 0.0f, 0.05f);
    ASSERT_LT(ray.direction.z, -0.9f);
}
