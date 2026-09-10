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
#include "Render/Extractor.hpp"
#include "Render/Picker.hpp"
#include "Render/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/MeshRenderer.hpp"
#include "World/World.hpp"

using namespace tests;

constexpr int WIDTH = 32;
constexpr int HEIGHT = 32;

class WorldDrawTest: public GPUTest
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

    [[nodiscard]] int targetWidth() const override
    {
        return WIDTH;
    }

    [[nodiscard]] int targetHeight() const override
    {
        return HEIGHT;
    }
};

//------------------------------------------------------------------------------
// The whole chain in one test: register a mesh and a material in the
// AssetManager, put a MeshRenderer on an Entity of the World, extract, render.
// The centre pixel of the framebuffer must be brighter than the clear colour.
//------------------------------------------------------------------------------
TEST_F(WorldDrawTest, RendersACubeThroughTheWholePipeline)
{
    assets::AssetManager assets;

    auto mesh = assets::makeCube();
    ASSERT_TRUE(bool(mesh)) << mesh.error();
    auto mesh_id = assets.addMesh("cube", mesh.take());
    ASSERT_TRUE(bool(mesh_id)) << mesh_id.error();

    auto lit = assets::makeLitMaterial();
    ASSERT_TRUE(bool(lit)) << lit.error();
    auto lit_id = assets.addMaterial("lit", lit.take());
    ASSERT_TRUE(bool(lit_id)) << lit_id.error();

    auto instance_id = assets.addMaterialInstance(
        "red",
        assets::MaterialInstance{ lit_id.value(),
                                  Vector3f(1.0f, 0.0f, 0.0f) });
    ASSERT_TRUE(bool(instance_id)) << instance_id.error();

    world::World world;
    scene::Scene scene(world, assets);

    world::Entity cube = world.create("cube");
    world.add(cube,
              world::MeshRenderer{ mesh_id.value(), instance_id.value() });

    world::Entity cam = world.create("camera");
    world.transform(cam).position = Vector3f(0.0f, 0.0f, 3.0f);
    world.add(cam, world::Camera{ world::Projection::Perspective, 60.0f, 1.0f,
                                  0.1f, 20.0f });
    scene.setActiveCamera(cam);

    world.update();
    auto snapshot = render::Extractor::extract(scene, WIDTH, HEIGHT);
    ASSERT_TRUE(bool(snapshot)) << snapshot.error();
    ASSERT_FALSE(snapshot.value().items.empty())
        << "extraction produced no items: the cube must have been culled or "
           "its ids were not resolved";

    gpu::PassDesc desc;
    desc.width = WIDTH;
    desc.height = HEIGHT;
    desc.color = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
    desc.clear_depth = true;
    auto pass = gpu::RenderPass::begin(desc);
    ASSERT_TRUE(bool(pass)) << pass.error();

    render::Renderer renderer;
    auto drawn = renderer.render(pass.value(), snapshot.value(), assets);
    ASSERT_TRUE(bool(drawn)) << drawn.error();

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    const std::size_t at =
        ((static_cast<std::size_t>(HEIGHT / 2) * WIDTH) + (WIDTH / 2)) * 4u;
    ASSERT_GT(static_cast<int>(picture.value()[at]), 20)
        << "the centre pixel is still black: the cube did not draw";
}

//------------------------------------------------------------------------------
TEST_F(WorldDrawTest, PicksTheCubeUnderTheCentrePixel)
{
    assets::AssetManager assets;
    auto mesh = assets::makeCube();
    ASSERT_TRUE(bool(mesh)) << mesh.error();
    auto mesh_id = assets.addMesh("cube", mesh.take());
    ASSERT_TRUE(bool(mesh_id)) << mesh_id.error();

    world::World world;
    scene::Scene scene(world, assets);

    world::Entity box = world.create("box");
    world.add(box, world::MeshRenderer{ mesh_id.value(), {} });

    world::Entity cam = world.create("cam");
    world.transform(cam).position = Vector3f(0.0f, 0.0f, 6.0f);
    world.add(cam, world::Camera{});
    scene.setActiveCamera(cam);
    world.update();

    auto snapshot = render::Extractor::extract(scene, WIDTH, HEIGHT);
    ASSERT_TRUE(bool(snapshot)) << snapshot.error();

    const auto hit = render::pickAt(scene,
                                    snapshot.value().camera,
                                    static_cast<float>(WIDTH) * 0.5f,
                                    static_cast<float>(HEIGHT) * 0.5f,
                                    static_cast<std::uint32_t>(WIDTH),
                                    static_cast<std::uint32_t>(HEIGHT));
    ASSERT_TRUE(hit.has_value());
    ASSERT_TRUE(hit->entity == box);
}
