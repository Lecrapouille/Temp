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
#include "Assets/Loaders/GltfLoader.hpp"
#include "Common/File.hpp"
#include "GPU/GPU.hpp"
#include "World/AnimationSystem.hpp"
#include "World/Components/Animator.hpp"
#include "World/Components/MeshRenderer.hpp"
#include "World/Components/SkinInstance.hpp"
#include "World/World.hpp"

#include <cmath>

using namespace tests;

namespace
{

std::string soldierPath()
{
    for (const char* root :
         { "external/OpenGLCppWrapper-data/",
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

    assets::AssetManager assets;
    world::World world;
    auto imported = assets::importGltf(path, assets, world);
    ASSERT_TRUE(bool(imported)) << imported.error();
    EXPECT_GE(imported.value().animations.size(), 3u);
    EXPECT_GE(imported.value().skin_count, 1u);
    EXPECT_TRUE(world.has<world::Animator>(imported.value().root));

    bool found_skin = false;
    auto const& skins = world.components<world::SkinInstance>();
    EXPECT_GE(skins.size(), 1u);
    auto const& renderers = world.components<world::MeshRenderer>();
    auto const meshes = renderers.components();
    for (std::size_t i = 0u; i < meshes.size(); ++i)
    {
        assets::MeshAsset const* mesh = assets.mesh(meshes[i].mesh);
        ASSERT_NE(mesh, nullptr);
        if (!mesh->rest_pose.empty())
        {
            found_skin = true;
            EXPECT_EQ(mesh->joint_indices.size(), mesh->rest_pose.size() * 4u);
        }
    }
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

    assets::AssetManager assets;
    world::World world;
    auto imported = assets::importGltf(path, assets, world);
    ASSERT_TRUE(bool(imported)) << imported.error();

    if (world.has<world::Animator>(imported.value().root))
    {
        world.get<world::Animator>(imported.value().root).playing = false;
    }
    world.update();
    ASSERT_TRUE(bool(world::AnimationSystem::skin(world, assets)));

    float worst = 0.0f;
    auto const& renderers = world.components<world::MeshRenderer>();
    auto const meshes = renderers.components();
    for (std::size_t i = 0u; i < meshes.size(); ++i)
    {
        assets::MeshAsset const* mesh = assets.mesh(meshes[i].mesh);
        if ((mesh == nullptr) || mesh->rest_pose.empty())
        {
            continue;
        }
        std::vector<assets::MeshVertex> posed(mesh->rest_pose.size());
        ASSERT_TRUE(bool(mesh->vertices.read(std::span<assets::MeshVertex>(posed))));
        for (std::size_t v = 0u; v < posed.size(); ++v)
        {
            const Vector3f d = posed[v].position - mesh->rest_pose[v].position;
            worst = std::max(worst, vector::norm(d));
        }
    }
    EXPECT_LT(worst, 8.0f) << "bind-pose skinning moved vertices too far";
}

//------------------------------------------------------------------------------
TEST_F(GltfAnimationTest, WalkMovesTheMesh)
{
    const std::string path = soldierPath();
    if (path.empty())
    {
        GTEST_SKIP() << "Soldier.glb is missing";
    }

    assets::AssetManager assets;
    world::World world;
    auto imported = assets::importGltf(path, assets, world);
    ASSERT_TRUE(bool(imported)) << imported.error();

    assets::MeshAsset const* body = nullptr;
    auto const& renderers = world.components<world::MeshRenderer>();
    auto const meshes = renderers.components();
    for (std::size_t i = 0u; i < meshes.size(); ++i)
    {
        assets::MeshAsset const* mesh = assets.mesh(meshes[i].mesh);
        if ((mesh != nullptr) && (mesh->rest_pose.size() > 100u))
        {
            body = mesh;
            break;
        }
    }
    ASSERT_NE(body, nullptr);

    std::vector<assets::MeshVertex> rest = body->rest_pose;
    ASSERT_TRUE(bool(world::AnimationSystem::tick(world, assets, 0.35f)));

    std::vector<assets::MeshVertex> posed(body->rest_pose.size());
    ASSERT_TRUE(bool(body->vertices.read(std::span<assets::MeshVertex>(posed))));
    float worst = 0.0f;
    for (std::size_t v = 0u; v < posed.size(); ++v)
    {
        const Vector3f d = posed[v].position - rest[v].position;
        worst = std::max(worst, vector::norm(d));
    }
    EXPECT_GT(worst, 1.0f) << "Walk clip left the bind pose unchanged";
}
