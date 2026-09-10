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

//! \brief How large the target of these tests is. Small, but big enough that a
//! triangle covering it can be told apart from one that missed.
constexpr int WIDTH = 32;
constexpr int HEIGHT = 32;

// ****************************************************************************
//! \brief A vertex with nothing but a place.
// ****************************************************************************
struct Corner
{
    Vector2f position;
};

//! \brief A triangle covering the whole target, wound counter clockwise.
const std::vector<Corner> BIG_TRIANGLE{ { { -1.0f, -1.0f } },
                                        { { 3.0f, -1.0f } },
                                        { { -1.0f, 3.0f } } };

//! \brief A square as two triangles, and the indices naming its corners. The
//! four corners are stored once and named six times, which is the whole point of
//! drawing by index.
const std::vector<Corner> SQUARE{ { { -1.0f, -1.0f } },
                                  { { 1.0f, -1.0f } },
                                  { { 1.0f, 1.0f } },
                                  { { -1.0f, 1.0f } } };

const std::vector<std::uint16_t> SQUARE_INDICES{ 0u, 1u, 2u, 0u, 2u, 3u };

constexpr const char* VERTEX = R"(#version 450 core
in vec2 position;
void main() { gl_Position = vec4(position, 0.0, 1.0); }
)";

//! \brief Paints everything a colour of its own, so that a pixel says whether the
//! draw arrived.
constexpr const char* RED_FRAGMENT = R"(#version 450 core
out vec4 oColor;
void main() { oColor = vec4(1.0, 0.0, 0.0, 1.0); }
)";

constexpr const char* GREEN_FRAGMENT = R"(#version 450 core
out vec4 oColor;
void main() { oColor = vec4(0.0, 1.0, 0.0, 1.0); }
)";

//! \brief Builds a triangle covering the target out of nothing but the index of
//! the vertex being run. What every full screen effect uses.
constexpr const char* GENERATED_VERTEX = R"(#version 450 core
void main()
{
    vec2 corners[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(corners[gl_VertexID], 0.0, 1.0);
}
)";

//! \brief The colour of one pixel of a picture read back, as red, green, blue and
//! alpha.
struct Pixel
{
    std::uint8_t red = 0u;
    std::uint8_t green = 0u;
    std::uint8_t blue = 0u;
    std::uint8_t alpha = 0u;

    [[nodiscard]] friend bool operator==(Pixel const& p_left,
                                         Pixel const& p_right)
    {
        return (p_left.red == p_right.red) && (p_left.green == p_right.green) &&
               (p_left.blue == p_right.blue) && (p_left.alpha == p_right.alpha);
    }
};

//! \brief Print a pixel the way a reader wants to see it when a test fails.
std::ostream& operator<<(std::ostream& p_stream, Pixel const& p_pixel)
{
    return p_stream << "rgba(" << int(p_pixel.red) << ", " << int(p_pixel.green)
                    << ", " << int(p_pixel.blue) << ", " << int(p_pixel.alpha)
                    << ")";
}

//! \brief One pixel of a picture read back.
Pixel pixelAt(std::vector<std::byte> const& p_picture,
              std::uint32_t p_x,
              std::uint32_t p_y)
{
    const std::size_t at =
        ((static_cast<std::size_t>(p_y) * WIDTH) + p_x) * 4u;
    return Pixel{ static_cast<std::uint8_t>(p_picture[at]),
                  static_cast<std::uint8_t>(p_picture[at + 1u]),
                  static_cast<std::uint8_t>(p_picture[at + 2u]),
                  static_cast<std::uint8_t>(p_picture[at + 3u]) };
}

constexpr Pixel RED{ 255u, 0u, 0u, 255u };
constexpr Pixel GREEN{ 0u, 255u, 0u, 255u };
constexpr Pixel BLACK{ 0u, 0u, 0u, 255u };

} // namespace

// ****************************************************************************
//! \brief A live device on an invisible window large enough to hold a picture.
// ****************************************************************************
class DrawTest: public GPUTest
{
protected:

    void SetUp() override
    {
        GPUTest::SetUp();
        auto ready = gpu::init(GPUContext::procAddress());
        ASSERT_TRUE(bool(ready)) << ready.error();

        m_program = gpu::Program::fromSources(VERTEX, RED_FRAGMENT).take();
        ASSERT_TRUE(m_program.valid());

        m_layout = GPU_LAYOUT(Corner, position);
    }

    void TearDown() override
    {
        m_pipeline.release();
        m_program.release();
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

    //! \brief A pipeline drawing filled triangles from a Corner.
    gpu::Pipeline& pipeline(gpu::RenderState const& p_state = {})
    {
        auto created = gpu::Pipeline::create<Corner>(m_program, m_layout, p_state);
        EXPECT_TRUE(bool(created)) << created.error();
        m_pipeline = created ? created.take() : gpu::Pipeline{};
        return m_pipeline;
    }

    //! \brief What a whole pass looks like, opened over the whole target.
    [[nodiscard]] static gpu::PassDesc wholeTarget()
    {
        return gpu::PassDesc{ .width = WIDTH, .height = HEIGHT, .target = {} };
    }

    gpu::Program m_program;
    gpu::Pipeline m_pipeline;
    gpu::VertexLayout m_layout;
};

//------------------------------------------------------------------------------
// A pass with nothing drawn in it still says what the target starts from, which
// is the smallest thing that runs and the whole of 01_ClearScreen.
//------------------------------------------------------------------------------
TEST_F(DrawTest, StartsTheTargetFromAColour)
{
    auto pass = gpu::RenderPass::begin(
        { .width = WIDTH,
          .height = HEIGHT,
          .color = { 0.0f, 1.0f, 0.0f, 1.0f },
          .target = {} });
    ASSERT_TRUE(bool(pass)) << pass.error();
    ASSERT_TRUE(pass.value().open());

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    ASSERT_EQ(picture.value().size(),
              static_cast<std::size_t>(WIDTH) * HEIGHT * 4u);
    ASSERT_EQ(pixelAt(picture.value(), 0u, 0u), GREEN);
    ASSERT_EQ(pixelAt(picture.value(), WIDTH - 1u, HEIGHT - 1u), GREEN);
}

//------------------------------------------------------------------------------
// The pass closes when it goes out of scope, so a frame returning early cannot
// leave one half open.
//------------------------------------------------------------------------------
TEST_F(DrawTest, ClosesThePassWhenItGoesOutOfScope)
{
    ASSERT_FALSE(gpu::inRenderPass());
    {
        auto pass = gpu::RenderPass::begin(wholeTarget());
        ASSERT_TRUE(bool(pass)) << pass.error();
        ASSERT_TRUE(gpu::inRenderPass());
        ASSERT_EQ(gpu::currentPass().width, WIDTH);
    }
    ASSERT_FALSE(gpu::inRenderPass());
    ASSERT_EQ(gpu::currentPass().width, 0u);
}

//------------------------------------------------------------------------------
TEST_F(DrawTest, RefusesToNestPasses)
{
    auto first = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(first)) << first.error();

    auto second = gpu::RenderPass::begin(wholeTarget());
    ASSERT_FALSE(bool(second));
    ASSERT_THAT(second.error(), HasSubstr("already open"));
}

//------------------------------------------------------------------------------
// A window whose size was read before it was shown gives zero, and a pass over
// nothing is worth refusing rather than drawing into a void.
//------------------------------------------------------------------------------
TEST_F(DrawTest, RefusesAPassOverNothing)
{
    auto pass = gpu::RenderPass::begin(
        { .width = 0u, .height = HEIGHT, .target = {} });
    ASSERT_FALSE(bool(pass));
    ASSERT_THAT(pass.error(), HasSubstr("minimised"));
    ASSERT_FALSE(gpu::inRenderPass());
}

//------------------------------------------------------------------------------
// The triangle, which is the hello world of the whole library: a buffer, a
// pipeline, a draw, and pixels that changed colour.
//------------------------------------------------------------------------------
TEST_F(DrawTest, DrawsATriangleThatReallyLandsOnTheTarget)
{
    auto vertices =
        gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                  gpu::BufferKind::Vertex,
                                  gpu::BufferUsage::Immutable)
            .take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::draw(pipeline(), vertices);
    ASSERT_TRUE(bool(drawn)) << drawn.error();

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    ASSERT_EQ(pixelAt(picture.value(), WIDTH / 2u, HEIGHT / 2u), RED);
}

//------------------------------------------------------------------------------
// A triangle that covers only part of the target leaves the rest as the pass
// started it, which is what says the pixels really came from the draw rather than
// from the clear.
//------------------------------------------------------------------------------
TEST_F(DrawTest, LeavesAloneWhatItDoesNotCover)
{
    // The bottom left quarter only.
    const std::vector<Corner> corner{ { { -1.0f, -1.0f } },
                                      { { 0.0f, -1.0f } },
                                      { { -1.0f, 0.0f } } };
    auto vertices = gpu::Buffer<Corner>::from(corner,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();
    ASSERT_TRUE(bool(gpu::draw(pipeline(), vertices)));

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    // Inside the triangle, near the corner it was built around.
    ASSERT_EQ(pixelAt(picture.value(), 1u, 1u), RED);
    // And the far corner, which it never reached.
    ASSERT_EQ(pixelAt(picture.value(), WIDTH - 1u, HEIGHT - 1u), BLACK);
}

//------------------------------------------------------------------------------
// The square drawn by index, which stores four corners and names six.
//------------------------------------------------------------------------------
TEST_F(DrawTest, DrawsFromIndicesNamingTheCorners)
{
    auto vertices = gpu::Buffer<Corner>::from(SQUARE,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();
    auto indices = gpu::Buffer<std::uint16_t>::from(SQUARE_INDICES,
                                                    gpu::BufferKind::Index,
                                                    gpu::BufferUsage::Immutable)
                       .take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::drawIndexed(pipeline(), vertices, indices);
    ASSERT_TRUE(bool(drawn)) << drawn.error();

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    // Both triangles of the square, one on each side of its diagonal.
    ASSERT_EQ(pixelAt(picture.value(), 2u, 2u), RED);
    ASSERT_EQ(pixelAt(picture.value(), WIDTH - 3u, HEIGHT - 3u), RED);
}

//------------------------------------------------------------------------------
// A full screen pass with no buffer, no layout and no upload: the shader makes
// its own corners out of the vertex index.
//------------------------------------------------------------------------------
TEST_F(DrawTest, DrawsWithNoVertexDataAtAll)
{
    auto program =
        gpu::Program::fromSources(GENERATED_VERTEX, GREEN_FRAGMENT).take();
    auto created = gpu::Pipeline::create(program, gpu::VertexLayout{});
    ASSERT_TRUE(bool(created)) << created.error();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::drawWithoutVertices(created.value(), 3u);
    ASSERT_TRUE(bool(drawn)) << drawn.error();

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    ASSERT_EQ(pixelAt(picture.value(), WIDTH / 2u, HEIGHT / 2u), GREEN);
}

//------------------------------------------------------------------------------
// A pipeline that reads attributes has nowhere to read them from when no buffer
// is given, which used to draw nothing and say nothing.
//------------------------------------------------------------------------------
TEST_F(DrawTest, RefusesToDrawWithoutVerticesWhenTheShaderWantsThem)
{
    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::drawWithoutVertices(pipeline(), 3u);
    ASSERT_FALSE(bool(drawn));
    ASSERT_THAT(drawn.error(), HasSubstr("nowhere for them to come from"));
}

//------------------------------------------------------------------------------
// Drawing outside a pass goes to whatever the driver last had bound, with
// whatever viewport. Refusing it is what makes the target of every draw
// something somebody chose.
//------------------------------------------------------------------------------
TEST_F(DrawTest, RefusesToDrawOutsideAPass)
{
    auto vertices = gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();

    auto drawn = gpu::draw(pipeline(), vertices);
    ASSERT_FALSE(bool(drawn));
    ASSERT_THAT(drawn.error(), HasSubstr("no pass is open"));
}

//------------------------------------------------------------------------------
// Four vertices drawn as triangles is one triangle and one vertex the device
// quietly ignores, so the missing triangle gets blamed on the shader.
//------------------------------------------------------------------------------
TEST_F(DrawTest, RefusesACountThatDoesNotMakeWholeTriangles)
{
    auto vertices = gpu::Buffer<Corner>::from(SQUARE,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::draw(pipeline(), vertices);
    ASSERT_FALSE(bool(drawn));
    ASSERT_THAT(drawn.error(), HasSubstr("1 would be left over"));
}

//------------------------------------------------------------------------------
// The same four vertices as a triangle strip make two triangles, which is the
// whole reason strips exist.
//------------------------------------------------------------------------------
TEST_F(DrawTest, AcceptsFourVerticesAsAStrip)
{
    auto vertices = gpu::Buffer<Corner>::from(SQUARE,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::draw(
        pipeline({ .primitive = gpu::Primitive::TriangleStrip }), vertices);
    ASSERT_TRUE(bool(drawn)) << drawn.error();
}

//------------------------------------------------------------------------------
TEST_F(DrawTest, RefusesToReadPastTheEndOfABuffer)
{
    auto vertices = gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::draw(pipeline(), vertices.handle(), 6u);
    ASSERT_FALSE(bool(drawn));
    ASSERT_THAT(drawn.error(), HasSubstr("the buffer holds 3"));
}

//------------------------------------------------------------------------------
// An index buffer handed over where a vertex buffer was meant. The driver places
// the two differently, so this is worth catching by name rather than letting it
// read nonsense.
//------------------------------------------------------------------------------
TEST_F(DrawTest, RefusesABufferOfTheWrongKind)
{
    auto indices = gpu::Buffer<std::uint16_t>::from(SQUARE_INDICES,
                                                    gpu::BufferKind::Index,
                                                    gpu::BufferUsage::Immutable)
                       .take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::draw(pipeline(), indices.handle(), 3u);
    ASSERT_FALSE(bool(drawn));
    ASSERT_THAT(drawn.error(), HasSubstr("created to hold index data"));
}

//------------------------------------------------------------------------------
TEST_F(DrawTest, RefusesVerticesGivenAsIndices)
{
    auto vertices = gpu::Buffer<Corner>::from(SQUARE,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::drawIndexed(pipeline(),
                                  vertices.handle(),
                                  vertices.handle(),
                                  gpu::IndexType::UInt16,
                                  6u);
    ASSERT_FALSE(bool(drawn));
    ASSERT_THAT(drawn.error(), HasSubstr("BufferKind::Index"));
}

//------------------------------------------------------------------------------
TEST_F(DrawTest, RefusesADrawOfNothing)
{
    auto vertices = gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::draw(pipeline(), vertices.handle(), 0u);
    ASSERT_FALSE(bool(drawn));
    ASSERT_THAT(drawn.error(), HasSubstr("never computed"));
}

//------------------------------------------------------------------------------
// A vertex array sends whatever changed on the way to the draw, which is the one
// call an example animating its vertices needs.
//------------------------------------------------------------------------------
TEST_F(DrawTest, SendsWhatChangedBeforeDrawingAVertexArray)
{
    gpu::VertexArray<Corner> corners{ std::span<const Corner>(BIG_TRIANGLE) };
    ASSERT_TRUE(corners.dirty());

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::draw(pipeline(), corners);
    ASSERT_TRUE(bool(drawn)) << drawn.error();
    ASSERT_FALSE(corners.dirty());

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    ASSERT_EQ(pixelAt(picture.value(), WIDTH / 2u, HEIGHT / 2u), RED);
}

//------------------------------------------------------------------------------
// Depth testing, which is what makes a near surface hide a far one. Drawn in the
// order that only comes out right if the test is doing its job.
//------------------------------------------------------------------------------
TEST_F(DrawTest, LetsTheNearerSurfaceWin)
{
    // Two triangles covering the target, the green one nearer than the red one,
    // drawn in the order that would give the wrong answer without a depth test.
    const std::vector<Corner> covering = BIG_TRIANGLE;
    auto vertices = gpu::Buffer<Corner>::from(covering,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();

    constexpr const char* NEAR_VERTEX = R"(#version 450 core
in vec2 position;
void main() { gl_Position = vec4(position, -0.5, 1.0); }
)";
    constexpr const char* FAR_VERTEX = R"(#version 450 core
in vec2 position;
void main() { gl_Position = vec4(position, 0.5, 1.0); }
)";

    auto near_program =
        gpu::Program::fromSources(NEAR_VERTEX, GREEN_FRAGMENT).take();
    auto far_program =
        gpu::Program::fromSources(FAR_VERTEX, RED_FRAGMENT).take();

    const gpu::RenderState tested{ .depth_test = true };
    auto nearer =
        gpu::Pipeline::create<Corner>(near_program, m_layout, tested).take();
    auto farther =
        gpu::Pipeline::create<Corner>(far_program, m_layout, tested).take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    ASSERT_TRUE(bool(gpu::draw(nearer, vertices)));
    // The far one comes second and must lose, which is only true if the distance
    // the first one recorded was kept.
    ASSERT_TRUE(bool(gpu::draw(farther, vertices)));

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    ASSERT_EQ(pixelAt(picture.value(), WIDTH / 2u, HEIGHT / 2u), GREEN);
}

//------------------------------------------------------------------------------
// Culling drops the triangles facing away, which halves the work on a closed
// mesh. A triangle wound the wrong way then disappears entirely, which is exactly
// what makes culling worth naming in a state rather than leaving switched on
// somewhere.
//------------------------------------------------------------------------------
TEST_F(DrawTest, DropsTheTrianglesFacingAway)
{
    // The same triangle wound clockwise, so it faces away from the camera.
    const std::vector<Corner> backwards{ { { -1.0f, -1.0f } },
                                         { { -1.0f, 3.0f } },
                                         { { 3.0f, -1.0f } } };
    auto vertices = gpu::Buffer<Corner>::from(backwards,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    ASSERT_TRUE(bool(gpu::draw(pipeline({ .cull = gpu::CullMode::Back }),
                               vertices)));

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    ASSERT_EQ(pixelAt(picture.value(), WIDTH / 2u, HEIGHT / 2u), BLACK);
}

//------------------------------------------------------------------------------
// A pass may draw into a corner of the target and leave the rest untouched, which
// is what a split view and a preview inset need.
//------------------------------------------------------------------------------
TEST_F(DrawTest, DrawsIntoOnlyPartOfTheTarget)
{
    auto vertices = gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();

    // Start the whole target from black.
    {
        auto whole = gpu::RenderPass::begin(wholeTarget());
        ASSERT_TRUE(bool(whole)) << whole.error();
    }
    // Then draw into its left half only, keeping what the first pass left.
    {
        auto half = gpu::RenderPass::begin({ .width = WIDTH / 2u,
                                             .height = HEIGHT,
                                             .clear_color = false,
                                             .clear_depth = false,
                                             .target = {} });
        ASSERT_TRUE(bool(half)) << half.error();
        ASSERT_TRUE(bool(gpu::draw(pipeline(), vertices)));

        // Reading beyond the pass is refused, since the pass is what says where
        // the picture is.
        auto too_wide = gpu::readPixels(0u, 0u, WIDTH, HEIGHT);
        ASSERT_FALSE(bool(too_wide));
        ASSERT_THAT(too_wide.error(), HasSubstr("outside the pass"));
    }
    // Reading the whole target needs a pass over the whole target, which must not
    // start it from a colour again.
    {
        auto whole = gpu::RenderPass::begin({ .width = WIDTH,
                                              .height = HEIGHT,
                                              .clear_color = false,
                                              .clear_depth = false,
                                             .target = {} });
        ASSERT_TRUE(bool(whole)) << whole.error();

        auto picture = gpu::readPixels();
        ASSERT_TRUE(bool(picture)) << picture.error();
        ASSERT_EQ(pixelAt(picture.value(), 1u, HEIGHT / 2u), RED);
        ASSERT_EQ(pixelAt(picture.value(), WIDTH - 2u, HEIGHT / 2u), BLACK);
    }
}

//------------------------------------------------------------------------------
// Being told to forget the state of the device does not change what is drawn. It
// cannot be tested by watching the driver, since the whole point is that the calls
// made are different while the picture is the same, so the picture is what is
// checked: after forgetting, everything is sent again, and a frame drawn that way
// has to come out identical to one drawn from the cache.
//------------------------------------------------------------------------------
TEST_F(DrawTest, DrawsTheSameAfterForgettingWhatTheDeviceWasTold)
{
    auto vertices = gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();

    // Once to fill the cache of the backend.
    {
        auto pass = gpu::RenderPass::begin(wholeTarget());
        ASSERT_TRUE(bool(pass)) << pass.error();
        ASSERT_TRUE(bool(gpu::draw(pipeline(), vertices)));
    }

    gpu::forgetRenderState();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();
    ASSERT_TRUE(bool(gpu::draw(pipeline(), vertices)));

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    ASSERT_EQ(pixelAt(picture.value(), WIDTH / 2u, HEIGHT / 2u), RED);
}

//------------------------------------------------------------------------------
// And it is safe to call when there is no device at all, because the code that
// calls it is a frame loop that has no reason to know whether the library was
// started.
//------------------------------------------------------------------------------
TEST(RenderStateWithoutDevice, ForgettingIsHarmlessWithNoDevice)
{
    gpu::forgetRenderState();
}

//------------------------------------------------------------------------------
// A pass into part of the target starts only that part from its colour. Worth a
// test of its own because the obvious implementation gets it wrong: the viewport
// says where a shape lands but does not hold a clear back, so a pass over the left
// half would wipe the whole window and the half drawn before it would vanish.
//------------------------------------------------------------------------------
TEST_F(DrawTest, ClearsOnlyItsOwnPartOfTheTarget)
{
    // The left half from red.
    {
        auto left = gpu::RenderPass::begin({ .width = WIDTH / 2u,
                                             .height = HEIGHT,
                                             .color = { 1.0f, 0.0f, 0.0f, 1.0f },
                                             .target = {} });
        ASSERT_TRUE(bool(left)) << left.error();
    }
    // The right half from green, which must leave the left half alone.
    {
        auto right = gpu::RenderPass::begin({ .x = WIDTH / 2u,
                                              .width = WIDTH / 2u,
                                              .height = HEIGHT,
                                              .color = { 0.0f, 1.0f, 0.0f, 1.0f },
                                              .target = {} });
        ASSERT_TRUE(bool(right)) << right.error();
    }

    auto whole = gpu::RenderPass::begin({ .width = WIDTH,
                                          .height = HEIGHT,
                                          .clear_color = false,
                                          .clear_depth = false,
                                          .target = {} });
    ASSERT_TRUE(bool(whole)) << whole.error();

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    ASSERT_EQ(pixelAt(picture.value(), 1u, HEIGHT / 2u), RED);
    ASSERT_EQ(pixelAt(picture.value(), (WIDTH / 2u) - 1u, HEIGHT / 2u), RED);
    ASSERT_EQ(pixelAt(picture.value(), WIDTH / 2u, HEIGHT / 2u), GREEN);
    ASSERT_EQ(pixelAt(picture.value(), WIDTH - 1u, HEIGHT / 2u), GREEN);
}

//------------------------------------------------------------------------------
// And nothing drawn during such a pass reaches outside it either, which is the
// other half of what makes a pass a region rather than a suggestion.
//------------------------------------------------------------------------------
TEST_F(DrawTest, KeepsWhatIsDrawnInsideItsOwnPart)
{
    auto vertices = gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();

    {
        auto whole = gpu::RenderPass::begin(wholeTarget());
        ASSERT_TRUE(bool(whole)) << whole.error();
    }
    {
        // A triangle covering everything, drawn during a pass over the left half.
        auto left = gpu::RenderPass::begin({ .width = WIDTH / 2u,
                                             .height = HEIGHT,
                                             .clear_color = false,
                                             .clear_depth = false,
                                             .target = {} });
        ASSERT_TRUE(bool(left)) << left.error();
        ASSERT_TRUE(bool(gpu::draw(pipeline(), vertices)));
    }

    auto whole = gpu::RenderPass::begin({ .width = WIDTH,
                                          .height = HEIGHT,
                                          .clear_color = false,
                                          .clear_depth = false,
                                          .target = {} });
    ASSERT_TRUE(bool(whole)) << whole.error();

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    ASSERT_EQ(pixelAt(picture.value(), (WIDTH / 2u) - 1u, HEIGHT / 2u), RED);
    ASSERT_EQ(pixelAt(picture.value(), WIDTH / 2u, HEIGHT / 2u), BLACK);
}

//------------------------------------------------------------------------------
// The counters an overlay watches. Reset every frame, because they are about a
// frame.
//------------------------------------------------------------------------------
TEST_F(DrawTest, CountsTheWorkOfAFrame)
{
    auto vertices = gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();

    gpu::resetFrameStatistics();
    ASSERT_EQ(gpu::frameStatistics().draw_calls, 0u);

    {
        auto pass = gpu::RenderPass::begin(wholeTarget());
        ASSERT_TRUE(bool(pass)) << pass.error();
        ASSERT_TRUE(bool(gpu::draw(pipeline(), vertices)));
        ASSERT_TRUE(bool(gpu::draw(m_pipeline, vertices)));
    }

    ASSERT_EQ(gpu::frameStatistics().passes, 1u);
    ASSERT_EQ(gpu::frameStatistics().draw_calls, 2u);
    ASSERT_EQ(gpu::frameStatistics().vertices, 6u);

    gpu::resetFrameStatistics();
    ASSERT_EQ(gpu::frameStatistics().draw_calls, 0u);
    ASSERT_EQ(gpu::frameStatistics().vertices, 0u);
}

//------------------------------------------------------------------------------
// The counters that must come back to where they started. This is the leak test
// the examples gallery runs continuously.
//------------------------------------------------------------------------------
TEST_F(DrawTest, CountsWhatTheDeviceIsHolding)
{
    const gpu::ResourceStatistics before = gpu::resourceStatistics();
    // The fixture holds one program and its two compiled stages.
    ASSERT_EQ(before.programs, 1u);
    ASSERT_EQ(before.buffers, 0u);
    ASSERT_EQ(before.pipelines, 0u);

    {
        auto vertices = gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                                  gpu::BufferKind::Vertex,
                                                  gpu::BufferUsage::Immutable)
                            .take();
        auto drawing = gpu::Pipeline::create<Corner>(m_program, m_layout).take();

        const gpu::ResourceStatistics during = gpu::resourceStatistics();
        ASSERT_EQ(during.buffers, 1u);
        ASSERT_EQ(during.pipelines, 1u);
        ASSERT_EQ(during.vertex_readers, 1u);
        ASSERT_EQ(during.buffer_bytes, BIG_TRIANGLE.size() * sizeof(Corner));
        ASSERT_THAT(during.toString(), HasSubstr("1 buffer(s)"));
    }

    const gpu::ResourceStatistics after = gpu::resourceStatistics();
    ASSERT_EQ(after.buffers, 0u);
    ASSERT_EQ(after.pipelines, 0u);
    ASSERT_EQ(after.vertex_readers, 0u);
    ASSERT_EQ(after.buffer_bytes, 0u);
}

//------------------------------------------------------------------------------
TEST_F(DrawTest, RefusesToReadPixelsOutsideAPass)
{
    auto picture = gpu::readPixels();
    ASSERT_FALSE(bool(picture));
    ASSERT_THAT(picture.error(), HasSubstr("no pass open"));
}

//------------------------------------------------------------------------------
// A pipeline built for one vertex struct and a buffer holding another read the
// right number of bytes from the wrong places, so something is drawn and it looks
// like a shader bug.
//------------------------------------------------------------------------------
TEST_F(DrawTest, RefusesABufferOfTheWrongVertex)
{
    struct Fat
    {
        Vector2f position;
        Vector2f padding;
    };

    auto wrong = gpu::Buffer<Fat>::create(3u,
                                          gpu::BufferKind::Vertex,
                                          gpu::BufferUsage::Dynamic)
                     .take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::draw(pipeline(), wrong);
    ASSERT_FALSE(bool(drawn));
    ASSERT_THAT(drawn.error(), HasSubstr("built for another vertex struct"));
}

//------------------------------------------------------------------------------
// One draw, two objects. The corners come from gl_VertexID; what changes per
// sprite lives in the buffer and is marked perInstance(). Mixing the two rates
// in one buffer is not something a single binding can say, so the layout is
// all instance and no vertex.
//------------------------------------------------------------------------------
TEST_F(DrawTest, DrawsEachInstanceAtItsOwnPlace)
{
    struct Sprite
    {
        Vector2f center;
        Vector2f extent;
        Vector4f color;
    };

    constexpr const char* vertex = R"(#version 450 core
in vec2 center;
in vec2 extent;
in vec4 color;
out vec4 vColor;

void main()
{
    vec2 corners[4] = vec2[4](vec2(-1.0, -1.0), vec2(1.0, -1.0),
                              vec2(-1.0, 1.0), vec2(1.0, 1.0));
    vColor = color;
    gl_Position = vec4(center + (corners[gl_VertexID] * extent), 0.0, 1.0);
}
)";
    constexpr const char* fragment = R"(#version 450 core
in vec4 vColor;
out vec4 oColor;
void main() { oColor = vColor; }
)";

    auto compiled = gpu::Program::fromSources(vertex, fragment);
    ASSERT_TRUE(bool(compiled)) << compiled.error();
    auto program = compiled.take();
    const gpu::VertexLayout layout = gpu::describe<Sprite>(
        gpu::field(&Sprite::center, "center").perInstance(),
        gpu::field(&Sprite::extent, "extent").perInstance(),
        gpu::field(&Sprite::color, "color").perInstance());

    gpu::RenderState state;
    state.primitive = gpu::Primitive::TriangleStrip;
    auto drawing = gpu::Pipeline::create<Sprite>(program, layout, state).take();
    ASSERT_TRUE(drawing.instanced());

    const std::vector<Sprite> sprites{
        { { -0.5f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.5f, 0.0f }, { 0.5f, 1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } }
    };
    auto instances = gpu::Buffer<Sprite>::from(
                         std::span<const Sprite>(sprites),
                         gpu::BufferKind::Vertex,
                         gpu::BufferUsage::Immutable)
                         .take();

    gpu::resetFrameStatistics();
    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::drawInstanced(drawing, instances, 4u, sprites.size());
    ASSERT_TRUE(bool(drawn)) << drawn.error();

    ASSERT_EQ(gpu::frameStatistics().draw_calls, 1u);
    ASSERT_EQ(gpu::frameStatistics().instances, 2u);
    ASSERT_EQ(gpu::frameStatistics().vertices, 8u);

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    ASSERT_EQ(pixelAt(picture.value(), 1u, HEIGHT / 2u), RED);
    ASSERT_EQ(pixelAt(picture.value(), WIDTH - 2u, HEIGHT / 2u), GREEN);
}

//------------------------------------------------------------------------------
TEST_F(DrawTest, RefusesADrawOfZeroInstances)
{
    auto vertices = gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::drawInstanced(pipeline(), vertices, 3u, 0u);
    ASSERT_FALSE(bool(drawn));
    ASSERT_THAT(drawn.error(), HasSubstr("zero instances"));
}

//------------------------------------------------------------------------------
TEST_F(DrawTest, RefusesMoreInstancesThanTheBufferHolds)
{
    struct Sprite
    {
        Vector2f center;
    };

    constexpr const char* vertex = R"(#version 450 core
in vec2 center;
void main()
{
    vec2 corners[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(center + corners[gl_VertexID], 0.0, 1.0);
}
)";

    auto compiled = gpu::Program::fromSources(vertex, RED_FRAGMENT);
    ASSERT_TRUE(bool(compiled)) << compiled.error();
    auto program = compiled.take();
    const gpu::VertexLayout layout = gpu::describe<Sprite>(
        gpu::field(&Sprite::center, "center").perInstance());
    auto drawing = gpu::Pipeline::create<Sprite>(program, layout).take();

    const std::vector<Sprite> one{ { { 0.0f, 0.0f } } };
    auto instances = gpu::Buffer<Sprite>::from(
                         std::span<const Sprite>(one),
                         gpu::BufferKind::Vertex,
                         gpu::BufferUsage::Immutable)
                         .take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::drawInstanced(drawing, instances, 3u, 2u);
    ASSERT_FALSE(bool(drawn));
    ASSERT_THAT(drawn.error(), HasSubstr("holds 1"));
}

//------------------------------------------------------------------------------
// The CPU never says how many vertices: the four words sitting on the device
// do. That is what a compute pass that culls writes, and what this reads.
//------------------------------------------------------------------------------
TEST_F(DrawTest, DrawsWhatTheCommandBufferAsks)
{
    auto vertices = gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();
    const gpu::DrawIndirectCommand command{ 3u, 1u, 0u, 0u };
    auto commands =
        gpu::Buffer<gpu::DrawIndirectCommand>::from(
            std::span<const gpu::DrawIndirectCommand>(&command, 1u),
            gpu::BufferKind::Storage,
            gpu::BufferUsage::Immutable)
            .take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::drawIndirect(pipeline(), vertices, commands);
    ASSERT_TRUE(bool(drawn)) << drawn.error();

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    ASSERT_EQ(pixelAt(picture.value(), WIDTH / 2u, HEIGHT / 2u), RED);
}

//------------------------------------------------------------------------------
TEST_F(DrawTest, RefusesACommandBufferThatIsTooSmall)
{
    auto vertices = gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();
    auto too_small = gpu::Buffer<std::uint32_t>::create(
                         2u, gpu::BufferKind::Storage, gpu::BufferUsage::Dynamic)
                         .take();

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn =
        gpu::drawIndirect(pipeline(), vertices.handle(), too_small.handle());
    ASSERT_FALSE(bool(drawn));
    ASSERT_THAT(drawn.error(), HasSubstr("16 bytes"));
}

//------------------------------------------------------------------------------
// A compute pass writes the four words; the draw never asks how many. The
// command barrier is what makes those words visible, the same way a vertex
// barrier makes a storage buffer drawable.
//------------------------------------------------------------------------------
TEST_F(DrawTest, DrawsWhatAComputePassWroteAsACommand)
{
    constexpr const char* fill = R"(#version 450 core
layout(local_size_x = 1) in;

layout(std430, binding = 0) buffer Command
{
    uint vertex_count;
    uint instance_count;
    uint first_vertex;
    uint first_instance;
};

void main()
{
    vertex_count = 3u;
    instance_count = 1u;
    first_vertex = 0u;
    first_instance = 0u;
}
)";

    auto vertices = gpu::Buffer<Corner>::from(BIG_TRIANGLE,
                                              gpu::BufferKind::Vertex,
                                              gpu::BufferUsage::Immutable)
                        .take();
    auto commands = gpu::Buffer<gpu::DrawIndirectCommand>::create(
                        1u,
                        gpu::BufferKind::Storage,
                        gpu::BufferUsage::Storage)
                        .take();

    auto compute = gpu::ComputeProgram::fromSource(fill).take();
    ASSERT_TRUE(bool(compute.bind("Command", commands)));
    ASSERT_TRUE(bool(compute.dispatch(1u)));
    gpu::barrier(gpu::Barrier::Command);

    auto pass = gpu::RenderPass::begin(wholeTarget());
    ASSERT_TRUE(bool(pass)) << pass.error();

    auto drawn = gpu::drawIndirect(pipeline(), vertices, commands);
    ASSERT_TRUE(bool(drawn)) << drawn.error();

    auto picture = gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    ASSERT_EQ(pixelAt(picture.value(), WIDTH / 2u, HEIGHT / 2u), RED);
}
