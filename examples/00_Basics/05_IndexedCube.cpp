//=============================================================================
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
// OpenGLCppWrapper is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#include "00_Basics/05_IndexedCube.hpp"

#include "Math/Transformation.hpp"

#include <array>
#include <cmath>
#include <vector>

//! \brief For the 60.0_deg the projection is asked for. An angle with its unit
//! written next to it cannot be passed in the wrong one, which is the whole reason
//! the maths here takes a radian_t rather than a float.
using namespace units::literals;

namespace examples
{

namespace
{

//! \brief The three matrices are separate uniforms and multiplied in the shader,
//! in the order every text on the subject uses. Combining them on the CPU would be
//! fewer multiplications, and is what a real renderer does; kept apart here because
//! this example is about what each one means.
constexpr const char* VERTEX_SHADER = R"(#version 450 core

in vec3 position;
in vec3 normal;
in vec3 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vNormal;
out vec3 vColor;

void main()
{
    // The rotation applies to the normal as it does to the position, but the
    // translation does not: a direction has no place. Taking the upper three by
    // three part of the matrix is what says so.
    vNormal = mat3(model) * normal;
    vColor = color;
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)";

constexpr const char* FRAGMENT_SHADER = R"(#version 450 core

in vec3 vNormal;
in vec3 vColor;

out vec4 oColor;

void main()
{
    // One fixed light, so that the faces are told apart by more than their colour
    // and the shape reads as a solid.
    const vec3 toLight = normalize(vec3(0.4, 0.8, 0.6));
    float lit = 0.25 + (0.75 * max(dot(normalize(vNormal), toLight), 0.0));
    oColor = vec4(vColor * lit, 1.0);
}
)";

//! \brief Half the width of the cube, so that it is one and a bit across and fits
//! in front of the camera.
constexpr float H = 0.6f;

} // namespace

//------------------------------------------------------------------------------
std::string IndexedCube::description() const
{
    return "Indices, so a corner is stored once and named several times. A depth "
           "test, so a near face covers a far one whatever the order. Back face "
           "culling, so half of a closed shape costs nothing. And three matrices, "
           "to place it, look at it and project it. The test and the culling belong "
           "to the pipeline, so nothing has to be switched off afterwards.";
}

//------------------------------------------------------------------------------
gpu::Status IndexedCube::setUp()
{
    GPU_TRY_ASSIGN(program,
                   gpu::Program::fromSources(VERTEX_SHADER, FRAGMENT_SHADER));
    m_program = std::move(program);

    std::vector<Vertex> corners;
    std::vector<std::uint16_t> indices;
    corners.reserve(24u);
    indices.reserve(36u);

    struct Face
    {
        Vector3f normal;
        Vector3f color;
    };

    const std::array<Face, 6u> faces{
        Face{ Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.9f, 0.3f, 0.3f) },
        Face{ Vector3f(0.0f, 0.0f, -1.0f), Vector3f(0.3f, 0.9f, 0.4f) },
        Face{ Vector3f(1.0f, 0.0f, 0.0f), Vector3f(0.3f, 0.5f, 0.9f) },
        Face{ Vector3f(-1.0f, 0.0f, 0.0f), Vector3f(0.9f, 0.8f, 0.3f) },
        Face{ Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.8f, 0.4f, 0.9f) },
        Face{ Vector3f(0.0f, -1.0f, 0.0f), Vector3f(0.4f, 0.9f, 0.9f) }
    };

    for (Face const& face : faces)
    {
        // Two directions along the face, found from its normal, so that the four
        // corners come out of the normal rather than being typed twenty four times.
        const Vector3f up =
            (std::abs(face.normal.y) > 0.5f) ? Vector3f(0.0f, 0.0f, 1.0f)
                                             : Vector3f(0.0f, 1.0f, 0.0f);
        const Vector3f right = vector::cross(up, face.normal);
        const Vector3f top = vector::cross(face.normal, right);

        const auto first = static_cast<std::uint16_t>(corners.size());
        // Counter clockwise seen from outside the cube, which is what makes the
        // culling drop the far faces rather than the near ones.
        corners.push_back(
            Vertex{ (face.normal - right - top) * H, face.normal, face.color });
        corners.push_back(
            Vertex{ (face.normal + right - top) * H, face.normal, face.color });
        corners.push_back(
            Vertex{ (face.normal + right + top) * H, face.normal, face.color });
        corners.push_back(
            Vertex{ (face.normal - right + top) * H, face.normal, face.color });

        // Four vertices, six indices: the two triangles of a face share a diagonal,
        // and the shared pair is stored once.
        indices.push_back(first);
        indices.push_back(static_cast<std::uint16_t>(first + 1u));
        indices.push_back(static_cast<std::uint16_t>(first + 2u));
        indices.push_back(first);
        indices.push_back(static_cast<std::uint16_t>(first + 2u));
        indices.push_back(static_cast<std::uint16_t>(first + 3u));
    }

    GPU_TRY_ASSIGN(vertices,
                   gpu::Buffer<Vertex>::from(std::span<const Vertex>(corners),
                                             gpu::BufferKind::Vertex,
                                             gpu::BufferUsage::Immutable));
    m_vertices = std::move(vertices);

    // Sixteen bits, because twenty four vertices are a long way below sixty five
    // thousand, and the device reads half as much for them. BufferKind::Index is
    // not decoration: passing a vertex buffer where indices were meant is one of
    // the mistakes the draw call refuses.
    GPU_TRY_ASSIGN(index_buffer,
                   gpu::Buffer<std::uint16_t>::from(
                       std::span<const std::uint16_t>(indices),
                       gpu::BufferKind::Index,
                       gpu::BufferUsage::Immutable));
    m_indices = std::move(index_buffer);

    gpu::RenderState state;
    state.depth_test = true;
    state.cull = gpu::CullMode::Back;

    const gpu::VertexLayout layout =
        GPU_LAYOUT(Vertex, position, normal, color);
    GPU_TRY_ASSIGN(pipeline,
                   gpu::Pipeline::create<Vertex>(m_program, layout, state));
    m_pipeline = std::move(pipeline);

    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status IndexedCube::draw(Frame const& p_frame)
{
    const Matrix44f identity(matrix::Identity);

    Matrix44f model = matrix::rotate(identity,
                                     units::angle::radian_t(p_frame.total * 0.7f),
                                     Vector3f(0.3f, 1.0f, 0.2f));

    const Matrix44f view = matrix::lookAt(Vector3f(0.0f, 0.0f, 3.0f),
                                          Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 1.0f, 0.0f));

    // Read from the frame and not remembered from set up: the window can be
    // resized, and a projection built once is how a cube ends up looking like a
    // brick.
    const Matrix44f projection =
        matrix::perspective(60.0_deg, p_frame.aspect(), 0.1f, 20.0f);

    GPU_TRY(m_program.set("model", model));
    GPU_TRY(m_program.set("view", view));
    GPU_TRY(m_program.set("projection", projection));

    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = Vector4f(0.07f, 0.07f, 0.1f, 1.0f);
    // The recorded distances are forgotten as well as the colour. Leaving them from
    // the previous frame is what makes a rotating solid disappear behind itself.
    desc.clear_depth = true;
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));

    // How many indices comes from the index buffer, and their size from their type.
    return gpu::drawIndexed(m_pipeline, m_vertices, m_indices);
}

} // namespace examples
