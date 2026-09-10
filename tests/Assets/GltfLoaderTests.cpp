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
#include "Assets/MeshAsset.hpp"
#include "GPU/GPU.hpp"
#include "Math/AABB.hpp"
#include "GPU/Statistics.hpp"
#include "Render/Extractor.hpp"
#include "Render/Renderer.hpp"
#include "Scene/Scene.hpp"
#include "World/Components/Camera.hpp"
#include "World/Components/MeshRenderer.hpp"
#include "World/World.hpp"

#include "Common/File.hpp"

#include <algorithm>
#include <cmath>

using namespace tests;

namespace
{

std::string dataPath(std::string const& p_name)
{
    for (const char* root :
         { "external/OpenGLCppWrapper-data/", "../external/OpenGLCppWrapper-data/" })
    {
        if (File::exist(root + p_name))
        {
            return root + p_name;
        }
    }
    return {};
}

} // namespace

class GltfLoaderTest: public GPUTest
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
        return 128;
    }

    [[nodiscard]] int targetHeight() const override
    {
        return 128;
    }
};

//------------------------------------------------------------------------------
TEST_F(GltfLoaderTest, LoadsTheKhronosDuck)
{
    const std::string path = dataPath("Duck.glb");
    if (path.empty())
    {
        GTEST_SKIP() << "Duck.glb is missing from external/OpenGLCppWrapper-data";
    }

    assets::AssetManager assets;
    world::World world;
    auto imported = assets::importGltf(path, assets, world);
    ASSERT_TRUE(bool(imported)) << imported.error();
    const assets::GltfImport scene = imported.take();
    ASSERT_TRUE(scene.root.valid());
    ASSERT_GT(scene.mesh_count, 0u);
    ASSERT_GT(scene.node_count, 0u);
    ASSERT_GT(assets.meshCount(), 0u);

    auto const& renderers = world.components<world::MeshRenderer>();
    ASSERT_GT(renderers.size(), 0u);
    {
        assets::MaterialInstance const* instance =
            assets.materialInstance(renderers.components()[0].material_instance);
        ASSERT_NE(instance, nullptr);
        ASSERT_TRUE(instance->material.valid())
            << "importGltf stored an empty MaterialId on the instance";
        ASSERT_NE(assets.material(instance->material), nullptr)
            << "the PBR Material was never registered with the AssetManager";
    }

    AABB bounds;
    auto const meshes = renderers.components();
    auto const entities = renderers.entities();
    for (std::size_t i = 0u; i < meshes.size(); ++i)
    {
        assets::MeshAsset const* mesh = assets.mesh(meshes[i].mesh);
        ASSERT_NE(mesh, nullptr);
        EXPECT_FALSE(mesh->local_bounds.empty()) << "mesh " << i;
        bounds = bounds.merged(
            mesh->local_bounds.transformed(world.worldMatrix(entities[i])));
    }
    ASSERT_FALSE(bounds.empty());
    const Vector3f extent = bounds.extent();
    EXPECT_TRUE(std::isfinite(bounds.center().x));
    EXPECT_TRUE(std::isfinite(bounds.center().y));
    EXPECT_TRUE(std::isfinite(bounds.center().z));
    // Duck.glb vertices are in centimetres; the root scale is 0.01, so the
    // world duck is about a metre tall — not 100 m and not a millimetre.
    EXPECT_GT(extent.y, 0.3f);
    EXPECT_LT(extent.y, 2.0f);

    world::Entity camera = world.create("Camera");
    world::Camera lens;
    lens.projection = world::Projection::Perspective;
    lens.fov_degrees = 45.0f;
    lens.near_plane = 0.05f;
    lens.far_plane = 50.0f;
    world.add(camera, lens);
    world.transform(camera).position =
        bounds.center() + Vector3f(0.0f, extent.y * 0.3f, extent.y * 4.0f);
    world.update();

    scene::Scene view(world, assets);
    view.setActiveCamera(camera);
    view.renderSettings().frustum_culling = false;
    auto uncull = render::Extractor::extract(view, 1280u, 720u);
    ASSERT_TRUE(bool(uncull)) << uncull.error();
    EXPECT_FALSE(uncull.value().items.empty())
        << "imported mesh never reached extract";

    view.renderSettings().frustum_culling = true;
    auto culled = render::Extractor::extract(view, 1280u, 720u);
    ASSERT_TRUE(bool(culled)) << culled.error();
    EXPECT_FALSE(culled.value().items.empty())
        << "Duck was frustum-culled despite sitting in front of the camera";
}

//------------------------------------------------------------------------------
TEST_F(GltfLoaderTest, RendersTheKhronosDuck)
{
    const std::string path = dataPath("Duck.glb");
    if (path.empty())
    {
        GTEST_SKIP() << "Duck.glb is missing from external/OpenGLCppWrapper-data";
    }

    assets::AssetManager assets;
    world::World world;
    auto imported = assets::importGltf(path, assets, world);
    ASSERT_TRUE(bool(imported)) << imported.error();

    AABB bounds;
    auto const& renderers = world.components<world::MeshRenderer>();
    auto const meshes = renderers.components();
    auto const entities = renderers.entities();
    ASSERT_GT(meshes.size(), 0u);
    assets::MeshAsset const* mesh = assets.mesh(meshes[0].mesh);
    ASSERT_NE(mesh, nullptr);
    ASSERT_GT(mesh->index_count, 0u);
    for (std::size_t i = 0u; i < meshes.size(); ++i)
    {
        assets::MeshAsset const* one = assets.mesh(meshes[i].mesh);
        ASSERT_NE(one, nullptr);
        bounds = bounds.merged(
            one->local_bounds.transformed(world.worldMatrix(entities[i])));
    }

    const Vector3f center = bounds.center();
    const Vector3f extent = bounds.extent();

    world::Entity camera = world.create("Camera");
    world::Camera lens;
    lens.projection = world::Projection::Perspective;
    lens.fov_degrees = 45.0f;
    lens.near_plane = std::max(extent.y * 0.01f, 0.01f);
    lens.far_plane = std::max(extent.y * 40.0f, 20.0f);
    world.add(camera, lens);
    world.transform(camera).position =
        center + Vector3f(0.0f, extent.y * 0.3f, extent.y * 4.0f);
    world.update();

    scene::Scene view(world, assets);
    view.setActiveCamera(camera);
    view.renderSettings().frustum_culling = false;
    view.environment().ambient = Vector3f(0.4f, 0.4f, 0.4f);

    constexpr std::uint32_t width = 128u;
    constexpr std::uint32_t height = 128u;
    auto snapshot = render::Extractor::extract(view, width, height);
    ASSERT_TRUE(bool(snapshot)) << snapshot.error();
    ASSERT_FALSE(snapshot.value().items.empty());

    gpu::PassDesc desc;
    desc.width = width;
    desc.height = height;
    desc.color = Vector4f(0.12f, 0.14f, 0.18f, 1.0f);
    desc.clear_depth = true;
    auto pass = gpu::RenderPass::begin(desc);
    ASSERT_TRUE(bool(pass)) << pass.error();

    render::Renderer renderer;
    auto drawn = renderer.render(pass.value(), snapshot.value(), assets);
    ASSERT_TRUE(bool(drawn)) << drawn.error();
    ASSERT_GE(gpu::frameStatistics().draw_calls, 1u)
        << "Renderer accepted the snapshot but issued no draw";

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();

    std::size_t lit = 0u;
    std::size_t orange = 0u;
    const auto& pixels = picture.value();
    for (std::size_t i = 0u; i + 3u < pixels.size(); i += 4u)
    {
        const int r = static_cast<int>(pixels[i]);
        const int g = static_cast<int>(pixels[i + 1u]);
        const int b = static_cast<int>(pixels[i + 2u]);
        if ((r > 40) || (g > 40) || (b > 50))
        {
            ++lit;
        }
        // The beak atlas cell is orange. Flipping the embedded PNG (the
        // OpenGL-file convention) maps that cell onto the body and the eye
        // onto the beak, so orange disappears from the frame.
        if ((r > 140) && (g < 180) && (b < 80) && (r > g + 20))
        {
            ++orange;
        }
    }

    const Vector3f cam_pos = world.transform(camera).position;
    ASSERT_GT(lit, 80u)
        << "Duck produced a draw but almost no visible pixels"
        << " items=" << snapshot.value().items.size()
        << " draws=" << gpu::frameStatistics().draw_calls
        << " indices=" << mesh->index_count
        << " bounds=[" << bounds.min.x << "," << bounds.min.y << ","
        << bounds.min.z << "]-[" << bounds.max.x << "," << bounds.max.y
        << "," << bounds.max.z << "]"
        << " cam=(" << cam_pos.x << "," << cam_pos.y << "," << cam_pos.z
        << ")"
        << " lit_pixels=" << lit
        << " orange_pixels=" << orange;
    ASSERT_GT(orange, 8u)
        << "Duck albedo is vertically flipped: the orange beak is missing"
        << " orange_pixels=" << orange
        << " lit_pixels=" << lit;
}
