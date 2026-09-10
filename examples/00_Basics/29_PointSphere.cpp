#include "00_Basics/29_PointSphere.hpp"

#include "GPU/Draw.hpp"
#include "GPU/RenderPass.hpp"
#include "Math/Transformation.hpp"
#include "Math/Units.hpp"

#include <cmath>
#include <vector>

using namespace units::literals;

namespace examples
{

namespace
{

constexpr char const* VERTEX = R"(#version 450 core
in vec3 position;
uniform mat4 mvp;
void main()
{
    gl_PointSize = 2.0;
    gl_Position = mvp * vec4(position, 1.0);
}
)";

constexpr char const* FRAGMENT = R"(#version 450 core
out vec4 oColor;
void main()
{
    oColor = vec4(0.85, 0.55, 0.35, 1.0);
}
)";

} // namespace

std::string PointSphere::description() const
{
    return "Legacy 06_IndexedSphere: a dense UV sphere drawn as GL_POINTS "
           "instead of triangles. Orbit is automatic so --check sees motion.";
}

gpu::Status PointSphere::setUp()
{
    constexpr std::uint32_t lon = 80u;
    constexpr std::uint32_t lat = 40u;
    constexpr float radius = 0.8f;
    std::vector<PointVertex> points;
    points.reserve(lon * lat);
    for (std::uint32_t i = 0u; i < lat; ++i)
    {
        const float v = static_cast<float>(i) / static_cast<float>(lat - 1u);
        const float phi = (v - 0.5f) * 3.14159265358979323846f;
        for (std::uint32_t j = 0u; j < lon; ++j)
        {
            const float u = static_cast<float>(j) / static_cast<float>(lon);
            const float theta = u * 2.0f * 3.14159265358979323846f;
            points.push_back(
                PointVertex{ Vector3f(radius * std::cos(phi) * std::cos(theta),
                                       radius * std::sin(phi),
                                       radius * std::cos(phi) * std::sin(theta)) });
        }
    }
    m_point_count = points.size();

    GPU_TRY_ASSIGN(buffer,
                   gpu::Buffer<PointVertex>::from(
                       std::span<const PointVertex>(points),
                       gpu::BufferKind::Vertex,
                       gpu::BufferUsage::Immutable));
    m_points = std::move(buffer);

    GPU_TRY_ASSIGN(program, gpu::Program::fromSources(VERTEX, FRAGMENT));
    m_program = std::move(program);
    const gpu::VertexLayout layout = GPU_LAYOUT(PointVertex, position);
    gpu::RenderState state;
    state.primitive = gpu::Primitive::Points;
    GPU_TRY_ASSIGN(
        pipeline,
        gpu::Pipeline::create<PointVertex>(m_program, layout, state));
    m_pipeline = std::move(pipeline);
    return gpu::success();
}

gpu::Status PointSphere::draw(Frame const& p_frame)
{
    const float aspect =
        (p_frame.height == 0u)
            ? 1.0f
            : (static_cast<float>(p_frame.width) /
               static_cast<float>(p_frame.height));
    const Matrix44f projection = matrix::perspective(
        units::angle::degree_t(60.0), aspect, 0.1f, 10.0f);
    const Matrix44f view =
        matrix::lookAt(Vector3f(0.0f, 0.0f, 2.5f), Vector3f(0.0f, 0.0f, 0.0f),
                       Vector3f(0.0f, 1.0f, 0.0f));
    const Matrix44f model =
        matrix::rotate(Matrix44f(matrix::Identity),
                       units::angle::radian_t(p_frame.total * 0.6f),
                       Vector3f(0.0f, 1.0f, 0.0f));
    const Matrix44f mvp = model * view * projection;

    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = Vector4f(0.05f, 0.07f, 0.10f, 1.0f);
    desc.clear_depth = true;
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
    GPU_TRY(m_program.set("mvp", mvp));
    return gpu::draw(m_pipeline, m_points.handle(), m_point_count);
}

} // namespace examples
