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

#include "Assets/Primitives.hpp"
#include "GPU/GPU.hpp"

using namespace tests;

class PrimitivesTest: public GPUTest
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
TEST_F(PrimitivesTest, SphereFitsInsideItsBounds)
{
    auto mesh = assets::makeSphere(0.5f, 8u, 12u);
    ASSERT_TRUE(bool(mesh)) << mesh.error();
    const assets::MeshAsset built = mesh.take();
    ASSERT_FALSE(built.local_bounds.empty());
    const Vector3f extent = built.local_bounds.extent();
    ASSERT_NEAR(extent.x, 0.5f, 1.0e-3f);
    ASSERT_NEAR(extent.y, 0.5f, 1.0e-3f);
    ASSERT_NEAR(extent.z, 0.5f, 1.0e-3f);
    ASSERT_GT(built.index_count, 0u);
}

//------------------------------------------------------------------------------
TEST_F(PrimitivesTest, RejectsAnInvalidSphere)
{
    const auto mesh = assets::makeSphere(0.0f, 8u, 12u);
    ASSERT_FALSE(mesh);
}

//------------------------------------------------------------------------------
TEST_F(PrimitivesTest, CylinderHasClosedSidesAndTwoCaps)
{
    constexpr std::uint32_t slices = 12u;
    auto mesh = assets::makeCylinder(0.5f, 2.0f, slices);
    ASSERT_TRUE(bool(mesh)) << mesh.error();
    const assets::MeshAsset built = mesh.take();
    // 2 triangles per side + 1 triangle per slice on each cap.
    ASSERT_EQ(built.index_count, (6u * slices) + (2u * 3u * slices));
    const Vector3f extent = built.local_bounds.extent();
    EXPECT_NEAR(extent.x, 0.5f, 1.0e-3f);
    EXPECT_NEAR(extent.y, 0.5f, 1.0e-3f);
    EXPECT_NEAR(extent.z, 1.0f, 1.0e-3f);
}

//------------------------------------------------------------------------------
TEST_F(PrimitivesTest, ConeTipSitsOnNegativeZAndHasABaseCapOnly)
{
    constexpr std::uint32_t slices = 16u;
    auto mesh = assets::makeCone(0.8f, 0.0f, 2.0f, slices);
    ASSERT_TRUE(bool(mesh)) << mesh.error();
    const assets::MeshAsset built = mesh.take();
    // Sides + the base disk. The collapsed tip is not capped.
    ASSERT_EQ(built.index_count, (6u * slices) + (3u * slices));
    EXPECT_NEAR(built.local_bounds.min.z, -1.0f, 1.0e-3f);
    EXPECT_NEAR(built.local_bounds.max.z, 1.0f, 1.0e-3f);
    EXPECT_NEAR(built.local_bounds.extent().x, 0.8f, 1.0e-3f);
}
