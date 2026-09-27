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
#include "Compages/Scene/MeshRenderer.hpp"
#include "Compages/Scene/Raycast.hpp"
#include "Compages/Scene/World.hpp"

//------------------------------------------------------------------------------
TEST(Raycast, PicksTheCloserOfTwoBoxes)
{
    scene::World world;
    const scene::EntityId near = world.create("near");
    const scene::EntityId far = world.create("far");
    world.add(near, scene::MeshRenderer{});
    world.add(far, scene::MeshRenderer{});
    world.update();

    const AABB near_box =
        AABB::fromCenterExtent(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(1.0f));
    const AABB far_box =
        AABB::fromCenterExtent(Vector3f(0.0f, 0.0f, -8.0f), Vector3f(1.0f));

    const Ray ray = Ray::fromPoints(Vector3f(0.0f, 0.0f, 10.0f),
                                    Vector3f(0.0f, 0.0f, 0.0f));
    const auto hit = scene::raycast(
        world,
        ray,
        [&](scene::EntityId p_entity, scene::MeshRenderer const&)
        {
            return (p_entity == near) ? near_box : far_box;
        });
    ASSERT_TRUE(hit.has_value());
    ASSERT_TRUE(hit->entity == near);
}

//------------------------------------------------------------------------------
TEST(Raycast, SkipsADisabledEntity)
{
    scene::World world;
    const scene::EntityId hidden = world.create("hidden");
    world.add(hidden, scene::MeshRenderer{});
    world.setEnabled(hidden, false);
    world.update();

    const Ray ray = Ray::fromPoints(Vector3f(0.0f, 0.0f, 10.0f),
                                    Vector3f(0.0f, 0.0f, 0.0f));
    const auto hit = scene::raycast(
        world,
        ray,
        [&](scene::EntityId, scene::MeshRenderer const&)
        {
            return AABB::fromCenterExtent(Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(1.0f));
        });
    ASSERT_FALSE(hit.has_value());
}

//------------------------------------------------------------------------------
TEST(WorldCamera, ScreenRayThroughTheCentreHitsTheOrigin)
{
    scene::World world;
    scene::AssetManager assets;
    scene::Scene scene(world, assets);

    scene::EntityId cam = world.create("cam");
    world.transform(cam).position = Vector3f(0.0f, 0.0f, 10.0f);
    world.add(cam, scene::Camera{});
    scene.activeCamera(cam);
    world.update();

    auto snapshot = scene::SceneExtractor::extract(scene, 1.0f);
    ASSERT_TRUE(bool(snapshot)) << snapshot.error();

    const Ray ray = snapshot.value().camera.screenRay(400.0f, 300.0f, 800u, 600u);
    // Centre of the view, looking along -Z: the ray should pass near the
    // origin and travel toward decreasing Z.
    ASSERT_NEAR(ray.direction.x, 0.0f, 0.05f);
    ASSERT_NEAR(ray.direction.y, 0.0f, 0.05f);
    ASSERT_LT(ray.direction.z, -0.9f);
}
