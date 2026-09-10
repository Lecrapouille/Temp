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

#include "01_Scientific/09_HeightMap.hpp"

#include "Math/Transformation.hpp"

#include <cmath>
#include <vector>

using namespace units::literals;

namespace examples
{

namespace
{

constexpr std::uint32_t SIDE = 320u;
constexpr float EXTENT = 1.6f;

constexpr const char* VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vNormal;
out float vHeight;

void main()
{
    vNormal = mat3(model) * normal;
    vHeight = position.y;
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)";

constexpr const char* FRAGMENT = R"(#version 450 core
in vec3 vNormal;
in float vHeight;
out vec4 oColor;

void main()
{
    const vec3 toLight = normalize(vec3(0.35, 0.8, 0.45));
    float lit = 0.2 + (0.8 * max(dot(normalize(vNormal), toLight), 0.0));
    vec3 low = vec3(0.15, 0.25, 0.45);
    vec3 high = vec3(0.85, 0.75, 0.45);
    float t = clamp(vHeight * 2.5 + 0.45, 0.0, 1.0);
    oColor = vec4(mix(low, high, t) * lit, 1.0);
}
)";

float heightAt(float p_x, float p_z, float p_time)
{
    return (0.18f * std::sin((6.0f * p_x) + p_time) *
            std::cos((6.0f * p_z) + (p_time * 0.7f))) +
           (0.07f * std::sin((12.0f * p_x * p_z) + (p_time * 1.3f)));
}

float coordOf(std::uint32_t p_index)
{
    return ((static_cast<float>(p_index) / static_cast<float>(SIDE - 1u)) *
            2.0f *
            EXTENT) -
           EXTENT;
}

} // namespace

//------------------------------------------------------------------------------
std::string HeightMap::description() const
{
    return "A hundred thousand vertices whose height is a function of place and "
           "time, decided on the CPU. VertexArray::modify() marks the whole "
           "surface dirty; the triangles naming the points never move, so they "
           "live in a Buffer written once. 03 sent one vertex. This sends the "
           "rest of the array.";
}

//------------------------------------------------------------------------------
gpu::Status HeightMap::setUp()
{
    GPU_TRY_ASSIGN(program, gpu::Program::fromSources(VERTEX, FRAGMENT));
    m_program = std::move(program);

    m_surface.resize(static_cast<std::size_t>(SIDE) * SIDE);
    auto vertices = m_surface.modify();
    for (std::uint32_t z = 0u; z < SIDE; ++z)
    {
        for (std::uint32_t x = 0u; x < SIDE; ++x)
        {
            const float px = coordOf(x);
            const float pz = coordOf(z);
            vertices[(static_cast<std::size_t>(z) * SIDE) + x] =
                Vertex{ Vector3f(px, heightAt(px, pz, 0.0f), pz),
                        Vector3f(0.0f, 1.0f, 0.0f) };
        }
    }

    std::vector<std::uint32_t> indices;
    indices.reserve(static_cast<std::size_t>(SIDE - 1u) * (SIDE - 1u) * 6u);
    for (std::uint32_t z = 0u; z < (SIDE - 1u); ++z)
    {
        for (std::uint32_t x = 0u; x < (SIDE - 1u); ++x)
        {
            const std::uint32_t here = (z * SIDE) + x;
            indices.push_back(here);
            indices.push_back(here + SIDE);
            indices.push_back(here + 1u);
            indices.push_back(here + 1u);
            indices.push_back(here + SIDE);
            indices.push_back(here + SIDE + 1u);
        }
    }

    GPU_TRY_ASSIGN(index_buffer,
                   gpu::Buffer<std::uint32_t>::from(
                       std::span<const std::uint32_t>(indices),
                       gpu::BufferKind::Index,
                       gpu::BufferUsage::Immutable));
    m_indices = std::move(index_buffer);

    gpu::RenderState state;
    state.depth_test = true;

    const gpu::VertexLayout layout = GPU_LAYOUT(Vertex, position, normal);
    GPU_TRY_ASSIGN(pipeline,
                   gpu::Pipeline::create<Vertex>(m_program, layout, state));
    m_pipeline = std::move(pipeline);
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status HeightMap::draw(Frame const& p_frame)
{
    const float step = (2.0f * EXTENT) / static_cast<float>(SIDE - 1u);
    auto vertices = m_surface.modify();
    for (std::uint32_t z = 0u; z < SIDE; ++z)
    {
        for (std::uint32_t x = 0u; x < SIDE; ++x)
        {
            const float px = coordOf(x);
            const float pz = coordOf(z);
            const float y = heightAt(px, pz, p_frame.total);
            // A normal from the function itself, so the lighting follows the
            // waves without a second pass over the neighbours.
            const Vector3f dx(step,
                              heightAt(px + step, pz, p_frame.total) - y,
                              0.0f);
            const Vector3f dz(0.0f,
                              heightAt(px, pz + step, p_frame.total) - y,
                              step);
            vertices[(static_cast<std::size_t>(z) * SIDE) + x] =
                Vertex{ Vector3f(px, y, pz), vector::normalize(vector::cross(dz, dx)) };
        }
    }

    const Matrix44f identity(matrix::Identity);
    const Matrix44f model = matrix::rotate(
        identity,
        units::angle::radian_t(p_frame.total * 0.25f),
        Vector3f(0.0f, 1.0f, 0.0f));
    const Matrix44f view = matrix::lookAt(Vector3f(0.0f, 1.8f, 3.2f),
                                          Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 1.0f, 0.0f));
    const Matrix44f projection =
        matrix::perspective(50.0_deg, p_frame.aspect(), 0.1f, 20.0f);

    GPU_TRY(m_program.set("model", model));
    GPU_TRY(m_program.set("view", view));
    GPU_TRY(m_program.set("projection", projection));

    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = Vector4f(0.06f, 0.07f, 0.1f, 1.0f);
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));

    return gpu::drawIndexed(m_pipeline, m_surface, m_indices);
}

} // namespace examples
