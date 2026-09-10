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
#include "Math/Vector.hpp"

#include <array>
#include <numeric>
#include <vector>

using namespace tests;

namespace
{

//! \brief An interleaved vertex, which is what the previous layer could not put
//! in a single buffer.
struct Vertex
{
    Vector3f position;
    Vector2f uv;
};

} // namespace

// ****************************************************************************
//! \brief A live device, on an invisible context.
// ****************************************************************************
class BufferTest: public GPUTest
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
TEST_F(BufferTest, ReservesMemoryOfTheRightSize)
{
    auto created = gpu::Buffer<Vertex>::create(
        1000u, gpu::BufferKind::Vertex, gpu::BufferUsage::Dynamic);
    ASSERT_TRUE(bool(created)) << created.error();

    auto buffer = created.take();
    ASSERT_TRUE(buffer.valid());
    ASSERT_EQ(buffer.count(), 1000u);
    ASSERT_EQ(buffer.bytes(), 1000u * sizeof(Vertex));
    ASSERT_EQ(buffer.kind(), gpu::BufferKind::Vertex);
    ASSERT_EQ(buffer.usage(), gpu::BufferUsage::Dynamic);
}

//------------------------------------------------------------------------------
// A count of zero is a count computed wrong, most often an empty vector nobody
// checked. Creating nothing and letting the first draw fail hides where it came
// from.
//------------------------------------------------------------------------------
TEST_F(BufferTest, RefusesAnEmptyBuffer)
{
    auto created = gpu::Buffer<Vertex>::create(
        0u, gpu::BufferKind::Vertex, gpu::BufferUsage::Dynamic);
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("zero"));
}

//------------------------------------------------------------------------------
// Immutable memory is the fastest the driver has, and the deal is that it is
// written once at creation. Asking for it empty can only end in a buffer that
// can never be filled, so it is refused where the mistake was made.
//------------------------------------------------------------------------------
TEST_F(BufferTest, RefusesAnEmptyImmutableBuffer)
{
    auto created = gpu::Buffer<Vertex>::create(
        4u, gpu::BufferKind::Vertex, gpu::BufferUsage::Immutable);
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("must be given its contents"));
}

//------------------------------------------------------------------------------
// The round trip that proves the bytes really reached the device: everything
// else in the library relies on this working.
//------------------------------------------------------------------------------
TEST_F(BufferTest, GivesBackWhatWasPutIn)
{
    std::vector<float> sent(256u);
    std::iota(sent.begin(), sent.end(), 1.0f);

    auto created = gpu::Buffer<float>::from(
        sent, gpu::BufferKind::Vertex, gpu::BufferUsage::Immutable);
    ASSERT_TRUE(bool(created)) << created.error();
    auto buffer = created.take();

    ASSERT_EQ(buffer.count(), sent.size());

    auto read = buffer.read();
    ASSERT_TRUE(bool(read)) << read.error();
    ASSERT_EQ(read.value(), sent);
}

//------------------------------------------------------------------------------
TEST_F(BufferTest, WritesPartOfABuffer)
{
    auto created = gpu::Buffer<int>::create(
        8u, gpu::BufferKind::Vertex, gpu::BufferUsage::Dynamic);
    ASSERT_TRUE(bool(created)) << created.error();
    auto buffer = created.take();

    const std::array<int, 8u> zeroes{ 0, 0, 0, 0, 0, 0, 0, 0 };
    ASSERT_TRUE(bool(buffer.write(zeroes)));

    // Three elements, starting at the fourth.
    const std::array<int, 3u> middle{ 7, 8, 9 };
    auto written = buffer.write(middle, 3u);
    ASSERT_TRUE(bool(written)) << written.error();

    auto read = buffer.read();
    ASSERT_TRUE(bool(read)) << read.error();
    const std::vector<int> expected{ 0, 0, 0, 7, 8, 9, 0, 0 };
    ASSERT_EQ(read.value(), expected);
}

//------------------------------------------------------------------------------
TEST_F(BufferTest, WritesASingleElement)
{
    const std::array<int, 4u> initial{ 1, 2, 3, 4 };
    auto created = gpu::Buffer<int>::from(
        initial, gpu::BufferKind::Vertex, gpu::BufferUsage::Dynamic);
    ASSERT_TRUE(bool(created)) << created.error();
    auto buffer = created.take();

    ASSERT_TRUE(bool(buffer.write(42, 2u)));

    auto read = buffer.read();
    ASSERT_TRUE(bool(read)) << read.error();
    const std::vector<int> expected{ 1, 2, 42, 4 };
    ASSERT_EQ(read.value(), expected);
}

//------------------------------------------------------------------------------
// The check the previous layer did not do. Writing past the end of a buffer is
// undefined behaviour on the device: the driver may ignore it, corrupt a
// neighbouring buffer, or kill the process. Refusing it here, with the two sizes
// in the message, turns a hang into a sentence.
//------------------------------------------------------------------------------
TEST_F(BufferTest, RefusesToWritePastTheEnd)
{
    auto created = gpu::Buffer<int>::create(
        4u, gpu::BufferKind::Vertex, gpu::BufferUsage::Dynamic);
    ASSERT_TRUE(bool(created)) << created.error();
    auto buffer = created.take();

    const std::array<int, 5u> too_many{ 1, 2, 3, 4, 5 };
    auto written = buffer.write(too_many);
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("past the end"));

    // Fits by itself, but not at that offset.
    const std::array<int, 2u> two{ 1, 2 };
    auto offset = buffer.write(two, 3u);
    ASSERT_FALSE(bool(offset));
    ASSERT_THAT(offset.error(), HasSubstr("past the end"));
}

//------------------------------------------------------------------------------
TEST_F(BufferTest, RefusesToWriteAnImmutableBuffer)
{
    const std::array<int, 4u> initial{ 1, 2, 3, 4 };
    auto created = gpu::Buffer<int>::from(
        initial, gpu::BufferKind::Vertex, gpu::BufferUsage::Immutable);
    ASSERT_TRUE(bool(created)) << created.error();
    auto buffer = created.take();

    auto written = buffer.write(0, 0u);
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("immutable"));
}

//------------------------------------------------------------------------------
// What a leak test watches. The count must come back to zero on its own, with
// nothing to call by hand.
//------------------------------------------------------------------------------
TEST_F(BufferTest, ReleasesItsMemoryWhenItGoesOutOfScope)
{
    ASSERT_EQ(gpu::liveBuffers(), 0u);
    {
        auto created = gpu::Buffer<int>::create(
            4u, gpu::BufferKind::Vertex, gpu::BufferUsage::Dynamic);
        ASSERT_TRUE(bool(created)) << created.error();
        auto buffer = created.take();
        ASSERT_EQ(gpu::liveBuffers(), 1u);
    }
    ASSERT_EQ(gpu::liveBuffers(), 0u);
}

//------------------------------------------------------------------------------
TEST_F(BufferTest, ReleasesEarlyWhenAsked)
{
    auto created = gpu::Buffer<int>::create(
        4u, gpu::BufferKind::Vertex, gpu::BufferUsage::Dynamic);
    ASSERT_TRUE(bool(created)) << created.error();
    auto buffer = created.take();

    buffer.release();
    ASSERT_FALSE(buffer.valid());
    ASSERT_EQ(buffer.count(), 0u);
    ASSERT_EQ(gpu::liveBuffers(), 0u);

    // Releasing twice is what a moved-from buffer does at the end of its scope.
    buffer.release();
    ASSERT_EQ(gpu::liveBuffers(), 0u);
}

//------------------------------------------------------------------------------
// Moving must hand the memory over, not share it: two owners would free the same
// memory twice.
//------------------------------------------------------------------------------
TEST_F(BufferTest, HandsMemoryOverWhenMoved)
{
    const std::array<int, 4u> initial{ 1, 2, 3, 4 };
    auto created = gpu::Buffer<int>::from(
        initial, gpu::BufferKind::Vertex, gpu::BufferUsage::Dynamic);
    ASSERT_TRUE(bool(created)) << created.error();
    auto first = created.take();
    const gpu::BufferHandle handle = first.handle();

    gpu::Buffer<int> second = std::move(first);
    ASSERT_EQ(gpu::liveBuffers(), 1u);
    ASSERT_FALSE(first.valid());
    ASSERT_TRUE(second.valid());
    ASSERT_EQ(second.handle(), handle);
    ASSERT_EQ(second.count(), 4u);

    auto read = second.read();
    ASSERT_TRUE(bool(read)) << read.error();
    ASSERT_EQ(read.value()[3], 4);
}

//------------------------------------------------------------------------------
// Assigning over a live buffer must free what was there, otherwise a buffer
// reassigned every frame leaks the whole scene.
//------------------------------------------------------------------------------
TEST_F(BufferTest, FreesWhatItHeldWhenAssigned)
{
    auto first = gpu::Buffer<int>::create(
                     4u, gpu::BufferKind::Vertex, gpu::BufferUsage::Dynamic)
                     .take();
    auto second = gpu::Buffer<int>::create(
                      8u, gpu::BufferKind::Vertex, gpu::BufferUsage::Dynamic)
                      .take();
    ASSERT_EQ(gpu::liveBuffers(), 2u);

    first = std::move(second);
    ASSERT_EQ(gpu::liveBuffers(), 1u);
    ASSERT_EQ(first.count(), 8u);
}

//------------------------------------------------------------------------------
// The mistake handles exist to catch: using memory that has already been given
// back. The pool answers the question instead of the program reading freed
// memory.
//------------------------------------------------------------------------------
TEST_F(BufferTest, RefusesToUseAHandleWhoseMemoryIsGone)
{
    auto buffer = gpu::Buffer<int>::create(
                      4u, gpu::BufferKind::Vertex, gpu::BufferUsage::Dynamic)
                      .take();
    const gpu::BufferHandle stale = buffer.handle();
    buffer.release();

    const std::array<int, 1u> one{ 1 };
    auto written = gpu::detail::writeBuffer(
        stale, 0u, sizeof(int), one.data());
    ASSERT_FALSE(bool(written));
    ASSERT_THAT(written.error(), HasSubstr("no longer exists"));

    ASSERT_FALSE(gpu::detail::bufferAlive(stale));
    ASSERT_EQ(gpu::detail::bufferBytes(stale), 0u);
}

//------------------------------------------------------------------------------
// A slot given back is reused, and the handle to its old occupant must not start
// naming the new one. This is what the generation counter in a handle is for.
//------------------------------------------------------------------------------
TEST_F(BufferTest, DoesNotConfuseANewBufferWithAnOldOne)
{
    gpu::BufferHandle stale;
    {
        auto first = gpu::Buffer<int>::create(
                         4u, gpu::BufferKind::Vertex, gpu::BufferUsage::Dynamic)
                         .take();
        stale = first.handle();
    }

    auto second = gpu::Buffer<int>::create(
                      4u, gpu::BufferKind::Vertex, gpu::BufferUsage::Dynamic)
                      .take();

    // The same slot, since the pool reuses it, but not the same buffer.
    ASSERT_EQ(second.handle().index(), stale.index());
    ASSERT_NE(second.handle(), stale);
    ASSERT_FALSE(gpu::detail::bufferAlive(stale));
    ASSERT_TRUE(gpu::detail::bufferAlive(second.handle()));
}

//------------------------------------------------------------------------------
// A uniform block is meant to be small, and the driver has a hard ceiling on it.
// Naming both the size asked for and the limit is more useful than a draw that
// quietly reads zeroes.
//------------------------------------------------------------------------------
TEST_F(BufferTest, RefusesAUniformBufferLargerThanTheDriverAllows)
{
    const std::size_t too_big =
        static_cast<std::size_t>(gpu::device().max_uniform_block_size) + 16u;

    auto created = gpu::Buffer<char>::create(
        too_big, gpu::BufferKind::Uniform, gpu::BufferUsage::Dynamic);
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("storage buffer instead"));
}

//------------------------------------------------------------------------------
// Without a device there is nowhere to put the memory. Saying so beats crashing
// inside a driver that has not been loaded.
//------------------------------------------------------------------------------
TEST_F(BufferTest, RefusesToCreateWithoutADevice)
{
    gpu::shutdown();

    auto created = gpu::Buffer<int>::create(
        4u, gpu::BufferKind::Vertex, gpu::BufferUsage::Dynamic);
    ASSERT_FALSE(bool(created));
    ASSERT_THAT(created.error(), HasSubstr("gpu::init()"));
}

//------------------------------------------------------------------------------
// A buffer that outlives the device is a bug in the caller, but it must not turn
// into a crash: the device frees what was forgotten, and the handle simply stops
// naming anything.
//------------------------------------------------------------------------------
TEST_F(BufferTest, SurvivesABufferOutlivingTheDevice)
{
    auto buffer = gpu::Buffer<int>::create(
                      4u, gpu::BufferKind::Vertex, gpu::BufferUsage::Dynamic)
                      .take();
    ASSERT_EQ(gpu::liveBuffers(), 1u);

    gpu::shutdown();
    ASSERT_EQ(gpu::liveBuffers(), 0u);
    ASSERT_FALSE(buffer.valid());

    const std::array<int, 1u> one{ 1 };
    ASSERT_FALSE(bool(buffer.write(one)));
    // And its destructor, running after this test, must not touch the driver.
}
