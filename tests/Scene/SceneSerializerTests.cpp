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

#include "GPUContext.hpp"

#include "Assets/AssetManager.hpp"
#include "Assets/Primitives.hpp"
#include "GPU/GPU.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SceneSerializer.hpp"
#include "World/Components/Light.hpp"
#include "World/Components/MeshRenderer.hpp"
#include "World/World.hpp"

#include <cstdio>
#include <string>

using namespace tests;

namespace
{

std::string tempScenePath()
{
    return std::string("/tmp/gloop_scene_test_") + std::to_string(getpid()) +
           ".json";
}

} // namespace

class SceneSerializerTest: public GPUTest
{
protected:

    void SetUp() override
    {
        GPUTest::SetUp();
        auto ready = gpu::init(GPUContext::procAddress());
        ASSERT_TRUE(bool(ready)) << ready.error();
    }

    void TearDown() override
    {
        gpu::shutdown();
        GPUTest::TearDown();
    }
};

//------------------------------------------------------------------------------
TEST_F(SceneSerializerTest, RoundTripsAMeshRenderer)
{
    assets::AssetManager assets;
    auto cube = assets::makeCube();
    ASSERT_TRUE(bool(cube));
    auto mesh = assets.addMesh("cube", cube.take());
    ASSERT_TRUE(bool(mesh));
    auto lit = assets::makeLitMaterial();
    ASSERT_TRUE(bool(lit));
    auto material = assets.addMaterial("lit", lit.take());
    ASSERT_TRUE(bool(material));
    auto instance = assets.addMaterialInstance(
        "red",
        assets::MaterialInstance{ material.value(), Vector3f(1.0f, 0.0f, 0.0f) });
    ASSERT_TRUE(bool(instance));

    world::World world;
    world::Entity root = world.create("Root");
    world::Entity box = world.create("Box");
    world.transform(box).position = Vector3f(1.0f, 2.0f, 3.0f);
    world.add(box, world::MeshRenderer{ mesh.value(), instance.value() });
    world.setParent(box, root);
    world.update();

    const std::string path = tempScenePath();
    ASSERT_TRUE(bool(scene::save(world, assets, path)));

    world::World loaded;
    auto roots = scene::load(loaded, assets, path);
    ASSERT_TRUE(bool(roots));
    ASSERT_EQ(roots.value().size(), 1u);
    world::Entity loaded_box = loaded.find(roots.value().front(), "Box");
    ASSERT_TRUE(loaded_box.valid());
    ASSERT_TRUE(loaded.has<world::MeshRenderer>(loaded_box));
    ASSERT_EQ(assets.meshName(loaded.get<world::MeshRenderer>(loaded_box).mesh),
              "cube");
    ASSERT_NEAR(loaded.transform(loaded_box).position.x, 1.0f, 1.0e-5f);

    std::remove(path.c_str());
}

//------------------------------------------------------------------------------
TEST_F(SceneSerializerTest, SaveSceneRoundTripsPresentationAndPointLight)
{
    assets::AssetManager assets;
    auto cube = assets::makeCube();
    ASSERT_TRUE(bool(cube));
    auto mesh = assets.addMesh("cube", cube.take());
    ASSERT_TRUE(bool(mesh));
    auto lit = assets::makeLitMaterial();
    ASSERT_TRUE(bool(lit));
    auto material = assets.addMaterial("lit", lit.take());
    ASSERT_TRUE(bool(material));
    auto instance = assets.addMaterialInstance(
        "red",
        assets::MaterialInstance{ material.value(), Vector3f(1.0f, 0.0f, 0.0f) });
    ASSERT_TRUE(bool(instance));

    world::World world;
    scene::Scene scene(world, assets);
    world::Entity lamp = world.create("Lamp");
    world.add(lamp, world::PointLight{ Vector3f(1.0f, 0.5f, 0.2f), 2.0f, 15.0f });
    world::Entity box = world.create("Box");
    world.add(box, world::MeshRenderer{ mesh.value(), instance.value() });
    world::Entity camera = world.create("Camera");
    scene.setActiveCamera(camera);
    scene.renderSettings().clear_color = Vector4f(0.1f, 0.2f, 0.3f, 1.0f);
    world.update();

    const std::string path = tempScenePath();
    ASSERT_TRUE(bool(scene::saveScene(scene, path)));

    world::World loaded_world;
    scene::Scene loaded_scene(loaded_world, assets);
    auto roots = scene::loadScene(loaded_scene, path);
    ASSERT_TRUE(bool(roots));
    world::Entity loaded_lamp{};
    world::Entity loaded_camera{};
    for (world::Entity root : roots.value())
    {
        if (loaded_world.name(root) == "Lamp")
        {
            loaded_lamp = root;
        }
        if (loaded_world.name(root) == "Camera")
        {
            loaded_camera = root;
        }
    }
    ASSERT_TRUE(loaded_lamp.valid());
    ASSERT_TRUE(loaded_camera.valid());
    ASSERT_TRUE(loaded_world.has<world::PointLight>(loaded_lamp));
    ASSERT_NEAR(loaded_scene.renderSettings().clear_color.y, 0.2f, 1.0e-5f);
    ASSERT_EQ(loaded_scene.activeCamera(), loaded_camera);

    std::remove(path.c_str());
}
