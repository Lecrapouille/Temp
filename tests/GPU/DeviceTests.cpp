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

#include "GPU/GPU.hpp"

#include <vector>

using namespace tests;

namespace
{

//! \brief Collects what the library logs, so a test can check what was said.
std::vector<std::pair<gpu::LogLevel, std::string>> g_messages;

void collectingLogger(gpu::LogLevel p_level, std::string_view p_message)
{
    g_messages.emplace_back(p_level, std::string(p_message));
}

} // namespace

// ****************************************************************************
//! \brief A device started on an invisible context, torn down after each test.
// ****************************************************************************
class DeviceTest: public GPUTest
{
protected:

    void SetUp() override
    {
        GPUTest::SetUp();
        g_messages.clear();
        gpu::logger(&collectingLogger);
    }

    void TearDown() override
    {
        gpu::shutdown();
        gpu::logger(nullptr);
        GPUTest::TearDown();
    }
};

//------------------------------------------------------------------------------
TEST_F(DeviceTest, StartsOnALiveContext)
{
    auto ready = gpu::init(GPUContext::procAddress());
    ASSERT_TRUE(bool(ready)) << ready.error();
    ASSERT_TRUE(gpu::initialized());
}

//------------------------------------------------------------------------------
// Without a loader there is nothing to load, and saying so is more useful than
// crashing inside the driver.
//------------------------------------------------------------------------------
TEST_F(DeviceTest, RefusesToStartWithoutALoader)
{
    auto ready = gpu::init(nullptr);
    ASSERT_FALSE(bool(ready));
    ASSERT_THAT(ready.error(), HasSubstr("symbol loader"));
    ASSERT_FALSE(gpu::initialized());
}

//------------------------------------------------------------------------------
// Starting twice would leak whatever the first run created, so it is refused
// rather than silently reinitializing.
//------------------------------------------------------------------------------
TEST_F(DeviceTest, RefusesToStartTwice)
{
    ASSERT_TRUE(bool(gpu::init(GPUContext::procAddress())));

    auto again = gpu::init(GPUContext::procAddress());
    ASSERT_FALSE(bool(again));
    ASSERT_THAT(again.error(), HasSubstr("twice"));
    // The first device is still the live one.
    ASSERT_TRUE(gpu::initialized());
}

//------------------------------------------------------------------------------
TEST_F(DeviceTest, StopsAndCanBeStartedAgain)
{
    ASSERT_TRUE(bool(gpu::init(GPUContext::procAddress())));
    gpu::shutdown();
    ASSERT_FALSE(gpu::initialized());

    // Idempotent: the examples gallery calls shutdown on a device that may
    // already be down.
    gpu::shutdown();
    ASSERT_FALSE(gpu::initialized());

    ASSERT_TRUE(bool(gpu::init(GPUContext::procAddress())));
    ASSERT_TRUE(gpu::initialized());
}

//------------------------------------------------------------------------------
// Every limit the library validates against must be a real value read from the
// driver. A zero here means a query was forgotten, and a validation using it
// would reject everything.
//------------------------------------------------------------------------------
TEST_F(DeviceTest, ReportsTheDriverLimits)
{
    ASSERT_TRUE(bool(gpu::init(GPUContext::procAddress())));
    gpu::DeviceInfo const& info = gpu::device();

    ASSERT_GE(info.version_major, 4);
    ASSERT_FALSE(info.vendor.empty());
    ASSERT_FALSE(info.renderer.empty());
    ASSERT_FALSE(info.version.empty());
    ASSERT_FALSE(info.shading_language_version.empty());

    // Values guaranteed by the OpenGL 4.5 specification itself.
    ASSERT_GE(info.max_vertex_attributes, 16);
    ASSERT_GE(info.max_texture_size, 1024);
    ASSERT_GE(info.max_texture_size_3d, 256);
    ASSERT_GE(info.max_texture_units, 48);
    ASSERT_GE(info.max_uniform_block_size, 16384);
    ASSERT_GE(info.uniform_buffer_offset_alignment, 1);
    ASSERT_GE(info.max_shader_storage_block_size, 1 << 24);

    ASSERT_GE(info.max_compute_work_group_invocations, 1024);
    for (std::size_t axis = 0u; axis < 3u; ++axis)
    {
        ASSERT_GE(info.max_compute_work_group_count[axis], 65535);
        ASSERT_GE(info.max_compute_work_group_size[axis], 64);
    }
}

//------------------------------------------------------------------------------
// The whole point of the debug callback is to replace asking for an error code
// after every call. If the driver refuses it, the library must say so out loud
// rather than pretend errors are being watched.
//------------------------------------------------------------------------------
TEST_F(DeviceTest, SaysWhetherTheDriverReportsItsOwnErrors)
{
    ASSERT_TRUE(bool(gpu::init(GPUContext::procAddress())));

    bool warned_about_no_debug_output = false;
    for (auto const& [level, message] : g_messages)
    {
        if ((level == gpu::LogLevel::Warning) &&
            (message.find("report its own errors") != std::string::npos))
        {
            warned_about_no_debug_output = true;
        }
    }

    ASSERT_EQ(gpu::device().debug_output, !warned_about_no_debug_output);
}

//------------------------------------------------------------------------------
// The name of the GPU in use belongs in the log of any application, so it is
// printed once at startup rather than left for the caller to fish out.
//------------------------------------------------------------------------------
TEST_F(DeviceTest, LogsWhichBackendAndGpuAreInUse)
{
    ASSERT_TRUE(bool(gpu::init(GPUContext::procAddress())));

    ASSERT_FALSE(g_messages.empty());
    ASSERT_EQ(g_messages[0].first, gpu::LogLevel::Info);
    ASSERT_THAT(g_messages[0].second, HasSubstr(gpu::device().renderer));
}

//------------------------------------------------------------------------------
// Driver messages are routed through the same logger as the library's own, so
// that a host application has one place to capture everything. This checks the
// routing; that the driver really uses it is checked once there are resources to
// misuse.
//------------------------------------------------------------------------------
TEST_F(DeviceTest, SendsMessagesWhereTheCallerAsked)
{
    ASSERT_TRUE(bool(gpu::init(GPUContext::procAddress())));

    const std::size_t before = g_messages.size();
    gpu::log(gpu::LogLevel::Error, "something to capture");

    ASSERT_EQ(g_messages.size(), before + 1u);
    ASSERT_EQ(g_messages.back().first, gpu::LogLevel::Error);
    ASSERT_EQ(g_messages.back().second, "something to capture");
}

//------------------------------------------------------------------------------
// Going back to the default logger must not leave a dangling pointer to a
// callback the caller has since discarded.
//------------------------------------------------------------------------------
TEST_F(DeviceTest, FallsBackToTheDefaultLogger)
{
    ASSERT_TRUE(bool(gpu::init(GPUContext::procAddress())));

    gpu::logger(nullptr);
    const std::size_t before = g_messages.size();
    gpu::log(gpu::LogLevel::Info, "goes to stderr, not to us");

    ASSERT_EQ(g_messages.size(), before);
}
