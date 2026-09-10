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

#include "main.hpp"

#include "GPU/Core/Layout.hpp"

namespace
{

//! \brief The everyday case: everything about one vertex, side by side.
struct Vertex
{
    Vector3f position;
    Vector3f normal;
    Vector2f uv;
};

//! \brief A vertex packing its colour in four bytes rather than four floats.
struct SmallVertex
{
    Vector3f position;
    Vector<std::uint8_t, 4u> color;
};

} // namespace

//------------------------------------------------------------------------------
// Formats are deduced from the C++ type, so a field renamed or retyped in the
// struct cannot go on being described as it was.
//------------------------------------------------------------------------------
TEST(Layout, ReadsTheFormatFromTheCppType)
{
    constexpr auto vec3 = gpu::formatOf<Vector3f>();
    ASSERT_EQ(vec3.scalar, gpu::ScalarType::Float);
    ASSERT_EQ(vec3.components, 3u);
    ASSERT_EQ(vec3.slots, 1u);
    ASSERT_FALSE(vec3.normalized);
    ASSERT_FALSE(vec3.as_integer);
    ASSERT_EQ(vec3.size(), 12u);

    constexpr auto scalar = gpu::formatOf<float>();
    ASSERT_EQ(scalar.components, 1u);
    ASSERT_EQ(scalar.size(), 4u);
}

//------------------------------------------------------------------------------
// A field of whole numbers reaches the shader as whole numbers unless asked
// otherwise, which is what an index or an identifier per vertex wants.
//------------------------------------------------------------------------------
TEST(Layout, KeepsWholeNumbersWholeByDefault)
{
    constexpr auto ivec = gpu::formatOf<Vector<std::int32_t, 2u>>();
    ASSERT_EQ(ivec.scalar, gpu::ScalarType::Int32);
    ASSERT_TRUE(ivec.as_integer);
    ASSERT_FALSE(ivec.normalized);
    ASSERT_EQ(ivec.glslType(), "ivec2");
}

//------------------------------------------------------------------------------
TEST(Layout, NamesTheGlslTypeAShaderMustDeclare)
{
    ASSERT_EQ(gpu::formatOf<float>().glslType(), "float");
    ASSERT_EQ(gpu::formatOf<Vector2f>().glslType(), "vec2");
    ASSERT_EQ(gpu::formatOf<Vector3f>().glslType(), "vec3");
    ASSERT_EQ(gpu::formatOf<Vector4f>().glslType(), "vec4");
    // Named, because a comma inside the template arguments would otherwise look
    // like a second argument to the macro.
    using UVec3 = Vector<std::uint32_t, 3u>;
    ASSERT_EQ(gpu::formatOf<UVec3>().glslType(), "uvec3");
    ASSERT_EQ(gpu::formatOf<std::int32_t>().glslType(), "int");
}

//------------------------------------------------------------------------------
// The offsets must be the ones the compiler chose, padding included, since those
// are the ones the GPU has to be told about.
//------------------------------------------------------------------------------
TEST(Layout, TakesTheOffsetsFromTheStructItself)
{
    const gpu::VertexLayout layout =
        gpu::describe<Vertex>(gpu::field(&Vertex::position, "aPosition"),
                              gpu::field(&Vertex::normal, "aNormal"),
                              gpu::field(&Vertex::uv, "aUV"));

    ASSERT_TRUE(bool(layout.validate())) << layout.validate().error();
    ASSERT_EQ(layout.fields().size(), 3u);
    ASSERT_EQ(layout.stride(), sizeof(Vertex));

    ASSERT_EQ(layout.fields()[0].offset, offsetof(Vertex, position));
    ASSERT_EQ(layout.fields()[1].offset, offsetof(Vertex, normal));
    ASSERT_EQ(layout.fields()[2].offset, offsetof(Vertex, uv));
}

//------------------------------------------------------------------------------
// One buffer holding whole vertices is what the previous design could not
// express: it insisted on one buffer per attribute, so the three fields of a
// vertex ended up far apart in memory.
//------------------------------------------------------------------------------
TEST(Layout, DescribesInterleavedVertices)
{
    const gpu::VertexLayout layout =
        gpu::describe<Vertex>(gpu::field(&Vertex::position, "aPosition"),
                              gpu::field(&Vertex::normal, "aNormal"),
                              gpu::field(&Vertex::uv, "aUV"));

    // All three fields step by the size of a whole vertex, which is exactly what
    // interleaved means.
    ASSERT_EQ(layout.stride(), 32u);
    ASSERT_EQ(layout.fields()[0].offset, 0u);
    ASSERT_EQ(layout.fields()[1].offset, 12u);
    ASSERT_EQ(layout.fields()[2].offset, 24u);
}

//------------------------------------------------------------------------------
TEST(Layout, FindsAFieldByTheNameTheShaderUses)
{
    const gpu::VertexLayout layout =
        gpu::describe<Vertex>(gpu::field(&Vertex::position, "aPosition"),
                              gpu::field(&Vertex::uv, "aUV"));

    ASSERT_NE(layout.find("aPosition"), nullptr);
    ASSERT_EQ(layout.find("aPosition")->offset, offsetof(Vertex, position));
    ASSERT_EQ(layout.find("aNormal"), nullptr);
}

//------------------------------------------------------------------------------
// A colour in four bytes instead of four floats saves three quarters of the
// memory and still arrives as a vec4.
//------------------------------------------------------------------------------
TEST(Layout, TurnsBytesIntoFractionsWhenAsked)
{
    const gpu::VertexLayout layout = gpu::describe<SmallVertex>(
        gpu::field(&SmallVertex::position, "aPosition"),
        gpu::field(&SmallVertex::color, "aColor").normalized());

    ASSERT_TRUE(bool(layout.validate())) << layout.validate().error();

    gpu::FieldDesc const* color = layout.find("aColor");
    ASSERT_NE(color, nullptr);
    ASSERT_EQ(color->format.scalar, gpu::ScalarType::UInt8);
    ASSERT_TRUE(color->format.normalized);
    // Normalizing means arriving as floating point, so not as a whole number.
    ASSERT_FALSE(color->format.as_integer);
    ASSERT_EQ(color->format.glslType(), "vec4");
    ASSERT_EQ(color->format.size(), 4u);
}

//------------------------------------------------------------------------------
TEST(Layout, MarksTheFieldsReadOncePerInstance)
{
    const gpu::VertexLayout layout = gpu::describe<Vertex>(
        gpu::field(&Vertex::position, "aPosition"),
        gpu::field(&Vertex::normal, "aOffset").perInstance());

    ASSERT_TRUE(layout.hasPerInstanceFields());
    ASSERT_FALSE(layout.find("aPosition")->per_instance);
    ASSERT_TRUE(layout.find("aOffset")->per_instance);
}

//------------------------------------------------------------------------------
TEST(Layout, CountsTheAttributeSlotsTheVertexTakes)
{
    const gpu::VertexLayout layout =
        gpu::describe<Vertex>(gpu::field(&Vertex::position, "aPosition"),
                              gpu::field(&Vertex::normal, "aNormal"),
                              gpu::field(&Vertex::uv, "aUV"));

    ASSERT_EQ(layout.slots(), 3u);
}

//------------------------------------------------------------------------------
// The macro is sugar and nothing more: it must produce exactly what writing the
// fields out by hand produces, with the member names as the shader names.
//------------------------------------------------------------------------------
TEST(Layout, MacroDescribesTheSameThingAsTheExplicitForm)
{
    const gpu::VertexLayout sugar = GPU_LAYOUT(Vertex, position, normal, uv);
    const gpu::VertexLayout explicit_form =
        gpu::describe<Vertex>(gpu::field(&Vertex::position, "position"),
                              gpu::field(&Vertex::normal, "normal"),
                              gpu::field(&Vertex::uv, "uv"));

    ASSERT_EQ(sugar.stride(), explicit_form.stride());
    ASSERT_EQ(sugar.fields().size(), explicit_form.fields().size());
    for (std::size_t i = 0u; i < sugar.fields().size(); ++i)
    {
        ASSERT_EQ(sugar.fields()[i].name, explicit_form.fields()[i].name);
        ASSERT_EQ(sugar.fields()[i].offset, explicit_form.fields()[i].offset);
        ASSERT_EQ(sugar.fields()[i].format.components,
                  explicit_form.fields()[i].format.components);
    }
}

//------------------------------------------------------------------------------
TEST(Layout, MacroWorksWithASingleField)
{
    const gpu::VertexLayout layout = GPU_LAYOUT(Vertex, position);

    ASSERT_EQ(layout.fields().size(), 1u);
    ASSERT_EQ(layout.fields()[0].name, "position");
}

//------------------------------------------------------------------------------
// Nothing forces the fields to be described in the order they appear, so the
// checks must not assume it either.
//------------------------------------------------------------------------------
TEST(Layout, AcceptsFieldsDescribedOutOfOrder)
{
    const gpu::VertexLayout layout =
        gpu::describe<Vertex>(gpu::field(&Vertex::uv, "aUV"),
                              gpu::field(&Vertex::position, "aPosition"));

    ASSERT_TRUE(bool(layout.validate())) << layout.validate().error();
    ASSERT_EQ(layout.fields()[0].name, "aUV");
    ASSERT_EQ(layout.fields()[0].offset, offsetof(Vertex, uv));
}

//------------------------------------------------------------------------------
// Describing the same member twice would have two shader attributes reading the
// same bytes, which is never what was meant.
//------------------------------------------------------------------------------
TEST(Layout, RefusesTwoFieldsSharingTheSameBytes)
{
    const gpu::VertexLayout layout =
        gpu::describe<Vertex>(gpu::field(&Vertex::position, "aPosition"),
                              gpu::field(&Vertex::position, "aSomethingElse"));

    auto status = layout.validate();
    ASSERT_FALSE(bool(status));
    ASSERT_THAT(status.error(), HasSubstr("overlap"));
    ASSERT_THAT(status.error(), HasSubstr("aPosition"));
    ASSERT_THAT(status.error(), HasSubstr("aSomethingElse"));
}

//------------------------------------------------------------------------------
TEST(Layout, RefusesTwoFieldsWithTheSameName)
{
    const gpu::VertexLayout layout =
        gpu::describe<Vertex>(gpu::field(&Vertex::position, "aThing"),
                              gpu::field(&Vertex::uv, "aThing"));

    auto status = layout.validate();
    ASSERT_FALSE(bool(status));
    ASSERT_THAT(status.error(), HasSubstr("aThing"));
}

//------------------------------------------------------------------------------
// Describing a member of one struct as belonging to another is a copy and paste
// mistake worth catching, and it shows up as a field running off the end.
//------------------------------------------------------------------------------
TEST(Layout, RefusesAFieldRunningPastTheEndOfTheVertex)
{
    // A field the size of a Vector3f, placed where only 4 bytes remain.
    std::vector<gpu::FieldDesc> fields{
        gpu::FieldDesc{ "aThing", gpu::formatOf<Vector3f>(), 4u, false }
    };
    const gpu::VertexLayout layout(std::move(fields), 8u);

    auto status = layout.validate();
    ASSERT_FALSE(bool(status));
    ASSERT_THAT(status.error(), HasSubstr("only 8 bytes"));
}

//------------------------------------------------------------------------------
TEST(Layout, RefusesAFieldWithoutAName)
{
    std::vector<gpu::FieldDesc> fields{
        gpu::FieldDesc{ "", gpu::formatOf<Vector3f>(), 0u, false }
    };
    const gpu::VertexLayout layout(std::move(fields), 12u);

    auto status = layout.validate();
    ASSERT_FALSE(bool(status));
    ASSERT_THAT(status.error(), HasSubstr("no name"));
}

//------------------------------------------------------------------------------
TEST(Layout, RefusesAFormatWithTooManyComponents)
{
    gpu::AttributeFormat format = gpu::formatOf<Vector4f>();
    format.components = 5u;
    ASSERT_FALSE(format.valid());

    std::vector<gpu::FieldDesc> fields{
        gpu::FieldDesc{ "aThing", format, 0u, false }
    };
    const gpu::VertexLayout layout(std::move(fields), 64u);

    ASSERT_FALSE(bool(layout.validate()));
    ASSERT_THAT(layout.validate().error(), HasSubstr("cannot read"));
}

//------------------------------------------------------------------------------
// Drawing without any vertex data is legitimate: a full screen effect generates
// its corners in the vertex shader.
//------------------------------------------------------------------------------
TEST(Layout, AcceptsHavingNoFieldsAtAll)
{
    const gpu::VertexLayout layout;

    ASSERT_TRUE(layout.empty());
    ASSERT_TRUE(bool(layout.validate()));
    ASSERT_EQ(layout.stride(), 0u);
    ASSERT_EQ(layout.slots(), 0u);
}

//------------------------------------------------------------------------------
// This text is what a reader sees when a shader and a layout disagree, so it must
// contain the things they need to compare.
//------------------------------------------------------------------------------
TEST(Layout, PrintsSomethingWorthReading)
{
    const gpu::VertexLayout layout = gpu::describe<SmallVertex>(
        gpu::field(&SmallVertex::position, "aPosition"),
        gpu::field(&SmallVertex::color, "aColor").normalized());

    const std::string text = layout.toString();

    ASSERT_THAT(text, HasSubstr("aPosition"));
    ASSERT_THAT(text, HasSubstr("vec3"));
    ASSERT_THAT(text, HasSubstr("aColor"));
    ASSERT_THAT(text, HasSubstr("normalized"));
    ASSERT_THAT(text, HasSubstr(std::to_string(sizeof(SmallVertex))));
}
