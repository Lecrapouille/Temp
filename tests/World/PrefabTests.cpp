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
#include "Assets/Prefabs.hpp"
#include "GPU/GPU.hpp"
#include "World/Components/MeshRenderer.hpp"
#include "World/Components/PrefabInstance.hpp"
#include "World/PrefabInstantiate.hpp"
#include "World/World.hpp"

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
    assets::AssetManager assets;
    auto cube = assets::makeCube();
    ASSERT_TRUE(bool(cube));
    ASSERT_TRUE(bool(assets.addMesh("cube", cube.take())));

    auto lit = assets::makeLitMaterial();
    ASSERT_TRUE(bool(lit));
    auto material = assets.addMaterial("lit", lit.take());
    ASSERT_TRUE(bool(material));
    ASSERT_TRUE(bool(assets.addMaterialInstance(
        "wood",
        assets::MaterialInstance{ material.value(), Vector3f(0.6f, 0.4f, 0.2f) })));
    ASSERT_TRUE(bool(assets.addMaterialInstance(
        "dark",
        assets::MaterialInstance{ material.value(), Vector3f(0.3f, 0.2f, 0.1f) })));
    ASSERT_TRUE(bool(assets.addMaterialInstance(
        "light",
        assets::MaterialInstance{ material.value(), Vector3f(0.9f, 0.9f, 0.8f) })));

    auto prefab_id = assets.addPrefab("robot", assets::makeRobotPrefab());
    ASSERT_TRUE(bool(prefab_id));

    world::World world;
    const std::size_t before = world.living();
    auto root = world::instantiate(world, assets, prefab_id.value());
    ASSERT_TRUE(bool(root));
    const world::Entity robot = root.take();
    ASSERT_TRUE(world.has<world::PrefabInstance>(robot));
    ASSERT_GT(world.living(), before);
    ASSERT_TRUE(world.find(robot, "Body/Head/HeadMesh").valid());
    ASSERT_TRUE(
        world.has<world::MeshRenderer>(world.find(robot, "Body/LeftLeg")));
}
