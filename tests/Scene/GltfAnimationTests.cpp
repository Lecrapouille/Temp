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
#include "Compages/Scene/Assets/GltfLoader.hpp"
#include "Compages/Core/File.hpp"
#include "Compages/GPU/GPU.hpp"
#include "Compages/Scene/Scene.hpp"
#include "Compages/Scene/AnimationSystem.hpp"
#include "Compages/Scene/Animator.hpp"
#include "Compages/Scene/MeshRenderer.hpp"
#include "Compages/Scene/SkinInstance.hpp"
#include "Compages/Scene/World.hpp"

#include <cmath>

using namespace tests;

namespace
{

std::string soldierPath()
{
    for (const char* root :
         { "external/Compages-data/",
           "../external/Compages-data/",
           "external/OpenGLCppWrapper-data/",
           "../external/OpenGLCppWrapper-data/" })
    {
        const std::string path = std::string(root) + "Soldier.glb";
        if (File::exist(path))
        {
            return path;
        }
    }
    if (File::exist("/home/qq/three.js/examples/models/gltf/Soldier.glb"))
    {
        return "/home/qq/three.js/examples/models/gltf/Soldier.glb";
    }
    return {};
}

} // namespace

class GltfAnimationTest: public GPUTest
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
TEST_F(GltfAnimationTest, LoadsSoldierClipsAndSkins)
{
    const std::string path = soldierPath();
    if (path.empty())
    {
        GTEST_SKIP() << "Soldier.glb is missing";
    }

    scene::AssetManager assets;
    scene::World world;
    scene::Scene scene(world, assets);
    auto loaded = assets.load(path);
    ASSERT_TRUE(bool(loaded)) << loaded.error();
    scene::Prefab const* prefab = assets.prefab(loaded.value());
    ASSERT_NE(prefab, nullptr);
    EXPECT_GE(prefab->animations.size(), 3u);
    auto imported = scene.instantiate(loaded.value());
    ASSERT_TRUE(bool(imported)) << imported.error();
    EXPECT_TRUE(world.has<scene::Animator>(imported.value().id()));

    bool found_skin = false;
    auto const skins = world.view<scene::SkinInstance>();
    EXPECT_GE(skins.size(), 1u);
    world.each<scene::MeshRenderer>(
        [&](scene::EntityId, scene::MeshRenderer const& p_renderer) {
        scene::MeshAsset const* mesh = assets.mesh(p_renderer.mesh);
        ASSERT_NE(mesh, nullptr);
        if (!mesh->rest_pose.empty())
        {
            found_skin = true;
            EXPECT_EQ(mesh->joint_indices.size(), mesh->rest_pose.size() * 4u);
        }
    });
    EXPECT_TRUE(found_skin);
}

//------------------------------------------------------------------------------
TEST_F(GltfAnimationTest, BindPoseKeepsRestVertices)
{
    const std::string path = soldierPath();
    if (path.empty())
    {
        GTEST_SKIP() << "Soldier.glb is missing";
    }

    scene::AssetManager assets;
    scene::World world;
    scene::Scene scene(world, assets);
    auto imported = scene.load(path);
    ASSERT_TRUE(bool(imported)) << imported.error();

    if (world.has<scene::Animator>(imported.value().id()))
    {
        world.get<scene::Animator>(imported.value().id()).playing = false;
    }
    world.update();
    ASSERT_TRUE(bool(scene::AnimationSystem::pose(world, assets)));

    float worst = 0.0f;
    world.each<scene::SkinInstance>(
        [&](scene::EntityId, scene::SkinInstance const& p_skin) {
        for (Matrix44f const& joint : p_skin.pose)
        {
            const Vector3f t(joint[3].x, joint[3].y, joint[3].z);
            worst = std::max(worst, vector::norm(t));
        }
    });
    EXPECT_LT(worst, 8.0f) << "bind-pose joint matrices translated too far";
}

//------------------------------------------------------------------------------
TEST_F(GltfAnimationTest, WalkMovesTheMesh)
{
    const std::string path = soldierPath();
    if (path.empty())
    {
        GTEST_SKIP() << "Soldier.glb is missing";
    }

    scene::AssetManager assets;
    scene::World world;
    scene::Scene scene(world, assets);
    auto imported = scene.load(path);
    ASSERT_TRUE(bool(imported)) << imported.error();

    scene::MeshAsset const* body = nullptr;
    world.each<scene::MeshRenderer>(
        [&](scene::EntityId, scene::MeshRenderer const& p_renderer) {
        if (body != nullptr)
        {
            return;
        }
        scene::MeshAsset const* mesh = assets.mesh(p_renderer.mesh);
        if ((mesh != nullptr) && (mesh->rest_pose.size() > 100u))
        {
            body = mesh;
        }
    });
    ASSERT_NE(body, nullptr);

    ASSERT_TRUE(bool(scene::AnimationSystem::pose(world, assets)));
    std::vector<Matrix44f> bind;
    auto const skins = world.view<scene::SkinInstance>();
    ASSERT_FALSE(skins.empty());
    auto const first_skin = *skins.begin();
    bind = skins.get<scene::SkinInstance>(first_skin).pose;

    ASSERT_TRUE(bool(scene::AnimationSystem::tick(world, assets, 0.35f)));

    float worst = 0.0f;
    auto const& after = skins.get<scene::SkinInstance>(first_skin).pose;
    ASSERT_EQ(after.size(), bind.size());
    for (std::size_t j = 0u; j < bind.size(); ++j)
    {
        const Vector3f d(
            after[j][3].x - bind[j][3].x,
            after[j][3].y - bind[j][3].y,
            after[j][3].z - bind[j][3].z);
        worst = std::max(worst, vector::norm(d));
    }
    EXPECT_GT(worst, 0.05f) << "Walk clip left the bind pose unchanged";
}
