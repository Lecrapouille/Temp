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

#include "GPUContext.hpp"

#include "Compages/Scene/Assets/AssetManager.hpp"
#include "Compages/Scene/Assets/Primitives.hpp"
#include "Compages/Scene/Assets/Prefabs.hpp"
#include "Compages/GPU/GPU.hpp"
#include "Compages/Scene/MeshRenderer.hpp"
#include "Compages/Scene/PrefabInstance.hpp"
#include "Compages/Scene/PrefabInstantiate.hpp"
#include "Compages/Scene/World.hpp"

using namespace tests;

class PrefabTest: public GPUTest
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
TEST_F(PrefabTest, InstantiatesTheRobotHierarchy)
{
    scene::AssetManager assets;
    auto cube = scene::makeCube();
    ASSERT_TRUE(bool(cube));
    ASSERT_TRUE(bool(assets.addMesh("box", cube.take())));

    auto lit = scene::makeLitMaterial();
    ASSERT_TRUE(bool(lit));
    auto material = assets.addMaterial("lit", lit.take());
    ASSERT_TRUE(bool(material));
    ASSERT_TRUE(bool(assets.addMaterialInstance(
        "wood",
        scene::MaterialInstance{ material.value(), Vector3f(0.6f, 0.4f, 0.2f) })));
    ASSERT_TRUE(bool(assets.addMaterialInstance(
        "dark",
        scene::MaterialInstance{ material.value(), Vector3f(0.3f, 0.2f, 0.1f) })));
    ASSERT_TRUE(bool(assets.addMaterialInstance(
        "light",
        scene::MaterialInstance{ material.value(), Vector3f(0.9f, 0.9f, 0.8f) })));

    auto prefab_id = assets.addPrefab("robot", scene::makeRobotPrefab());
    ASSERT_TRUE(bool(prefab_id));

    scene::World world;
    const std::size_t before = world.living();
    auto root = scene::instantiate(world, assets, prefab_id.value());
    ASSERT_TRUE(bool(root));
    const scene::EntityId robot = root.take();
    ASSERT_TRUE(world.has<scene::PrefabInstance>(robot));
    ASSERT_GT(world.living(), before);
    ASSERT_TRUE(world.find(robot, "Body/Head/HeadMesh").valid());
    ASSERT_TRUE(
        world.has<scene::MeshRenderer>(world.find(robot, "Body/LeftLeg")));
}
