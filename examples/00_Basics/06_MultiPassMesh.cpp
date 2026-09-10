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

#include "00_Basics/06_MultiPassMesh.hpp"

#include "Math/Transformation.hpp"

#include <array>
#include <cmath>
#include <vector>

using namespace units::literals;

namespace examples
{

namespace
{

//! \brief Reads every field of the vertex. The other two shaders deliberately
//! do not, so that the linker drops what they never use and the three pipelines
//! do not all share one way of reading. The block is written out in each shader
//! rather than pasted: a `const char*` does not join with a literal next to it.
constexpr const char* LIT_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;
in vec3 color;

layout(std140, binding = 0) uniform Transforms
{
    mat4 projection;
    mat4 view;
    mat4 model;
} u;

out vec3 vNormal;
out vec3 vColor;

void main()
{
    vNormal = mat3(u.model) * normal;
    vColor = color;
    gl_Position = u.projection * u.view * u.model * vec4(position, 1.0);
}
)";

constexpr const char* LIT_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
in vec3 vColor;
out vec4 oColor;

void main()
{
    const vec3 toLight = normalize(vec3(0.4, 0.8, 0.6));
    float lit = 0.25 + (0.75 * max(dot(normalize(vNormal), toLight), 0.0));
    oColor = vec4(vColor * lit, 1.0);
}
)";

//! \brief Colour is declared nowhere. The linker drops it from the vertex, so
//! this pipeline reads a shorter prefix of the same buffer.
constexpr const char* NORMALS_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;

layout(std140, binding = 0) uniform Transforms
{
    mat4 projection;
    mat4 view;
    mat4 model;
} u;

out vec3 vNormal;

void main()
{
    vNormal = mat3(u.model) * normal;
    gl_Position = u.projection * u.view * u.model * vec4(position, 1.0);
}
)";

constexpr const char* NORMALS_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
out vec4 oColor;

void main()
{
    oColor = vec4(normalize(vNormal) * 0.5 + 0.5, 1.0);
}
)";

//! \brief Position only. A depth-only pass or a shadow pass looks like this:
//! same vertices, a shader that has no use for the rest.
constexpr const char* WIRE_VERTEX = R"(#version 450 core
in vec3 position;

layout(std140, binding = 0) uniform Transforms
{
    mat4 projection;
    mat4 view;
    mat4 model;
} u;

void main()
{
    gl_Position = u.projection * u.view * u.model * vec4(position, 1.0);
}
)";

constexpr const char* WIRE_FRAGMENT = R"(#version 450 core
out vec4 oColor;

void main()
{
    oColor = vec4(0.85, 0.85, 0.9, 1.0);
}
)";

constexpr float H = 0.55f;

} // namespace

//------------------------------------------------------------------------------
std::string MultiPassMesh::description() const
{
    return "One Buffer<Vertex>, three pipelines, one uniform block. The left "
           "pass lights the mesh, the middle paints its normals, the right "
           "draws only the edges. They share the vertices and the matrices; "
           "they do not share a way of reading a vertex, because each shader "
           "asks for a different subset and the linker drops the rest. The "
           "counters below should show three pipelines, more than one vertex "
           "reader, and three buffers: vertices, indices, and the block.";
}

//------------------------------------------------------------------------------
gpu::Status MultiPassMesh::makePipeline(gpu::Program& p_program,
                                        gpu::Pipeline& p_pipeline,
                                        char const* p_vertex,
                                        char const* p_fragment,
                                        gpu::RenderState const& p_state)
{
    GPU_TRY_ASSIGN(program, gpu::Program::fromSources(p_vertex, p_fragment));

    const gpu::VertexLayout layout =
        GPU_LAYOUT(Vertex, position, normal, color);
    GPU_TRY_ASSIGN(pipeline,
                   gpu::Pipeline::create<Vertex>(program, layout, p_state));

    // The program has to outlive the pipeline: the pipeline names it, it does
    // not own it.
    p_program = std::move(program);
    p_pipeline = std::move(pipeline);
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status MultiPassMesh::setUp()
{
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
        const Vector3f up = (std::abs(face.normal.y) > 0.5f)
                                ? Vector3f(0.0f, 0.0f, 1.0f)
                                : Vector3f(0.0f, 1.0f, 0.0f);
        const Vector3f right = vector::cross(up, face.normal);
        const Vector3f top = vector::cross(face.normal, right);

        const auto first = static_cast<std::uint16_t>(corners.size());
        corners.push_back(
            Vertex{ (face.normal - right - top) * H, face.normal, face.color });
        corners.push_back(
            Vertex{ (face.normal + right - top) * H, face.normal, face.color });
        corners.push_back(
            Vertex{ (face.normal + right + top) * H, face.normal, face.color });
        corners.push_back(
            Vertex{ (face.normal - right + top) * H, face.normal, face.color });

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

    GPU_TRY_ASSIGN(index_buffer,
                   gpu::Buffer<std::uint16_t>::from(
                       std::span<const std::uint16_t>(indices),
                       gpu::BufferKind::Index,
                       gpu::BufferUsage::Immutable));
    m_indices = std::move(index_buffer);

    gpu::RenderState solid;
    solid.depth_test = true;
    solid.cull = gpu::CullMode::Back;

    gpu::RenderState wire;
    wire.depth_test = true;
    wire.polygon = gpu::PolygonMode::Line;
    // Both sides, so the far edges stay visible instead of being culled as the
    // back of a filled solid would be.
    wire.cull = gpu::CullMode::None;

    GPU_TRY(makePipeline(m_lit_program, m_lit, LIT_VERTEX, LIT_FRAGMENT, solid));
    GPU_TRY(makePipeline(
        m_normals_program, m_normals, NORMALS_VERTEX, NORMALS_FRAGMENT, solid));
    GPU_TRY(makePipeline(
        m_wire_program, m_wire, WIRE_VERTEX, WIRE_FRAGMENT, wire));

    // Any of the three programs will do: they declare the same block on the same
    // point. The other two are told nothing, because a binding point belongs to
    // the device, not to a program.
    GPU_TRY_ASSIGN(block,
                   gpu::TypedUniformBlock<Transforms>::create(m_lit_program,
                                                              "Transforms"));
    m_transforms = std::move(block);

    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status MultiPassMesh::draw(Frame const& p_frame)
{
    const Matrix44f identity(matrix::Identity);

    Transforms transforms;
    transforms.model = matrix::rotate(
        identity,
        units::angle::radian_t(p_frame.total * 0.6f),
        Vector3f(0.35f, 1.0f, 0.15f));
    transforms.view = matrix::lookAt(Vector3f(0.0f, 0.0f, 2.6f),
                                     Vector3f(0.0f, 0.0f, 0.0f),
                                     Vector3f(0.0f, 1.0f, 0.0f));
    // Each third of the window is a camera of its own, so the aspect is that of
    // one third rather than of the whole, or the cube would look like a brick.
    transforms.projection = matrix::perspective(
        55.0_deg, p_frame.aspect() / 3.0f, 0.1f, 20.0f);

    m_transforms.assign(transforms);
    GPU_TRY(m_transforms.bind());

    const std::uint32_t third = p_frame.width / 3u;
    const std::uint32_t last = p_frame.width - (2u * third);
    gpu::Pipeline const* passes[3] = { &m_lit, &m_normals, &m_wire };
    const std::uint32_t widths[3] = { third, third, last };

    std::uint32_t x = 0u;
    for (std::size_t i = 0u; i < 3u; ++i)
    {
        gpu::PassDesc desc;
        desc.x = x;
        desc.width = widths[i];
        desc.height = p_frame.height;
        desc.color = Vector4f(0.07f, 0.07f, 0.1f, 1.0f);
        desc.clear_depth = true;
        GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));

        GPU_TRY(gpu::drawIndexed(*passes[i], m_vertices, m_indices));
        x += widths[i];
    }

    return gpu::success();
}

} // namespace examples
