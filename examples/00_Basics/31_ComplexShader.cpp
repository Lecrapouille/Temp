#include "00_Basics/31_ComplexShader.hpp"

#include "GPU/Draw.hpp"
#include "GPU/RenderPass.hpp"

namespace examples
{

namespace
{

constexpr char const* VERTEX = R"(#version 450 core
in vec2 position;
in vec2 uv;
out vec2 vUv;
void main()
{
    vUv = uv;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

// Port of legacy 11_ComplexShader.fs (ShaderFrog Universe Nursery).
constexpr char const* FRAGMENT = R"(#version 450 core
in vec2 vUv;
uniform vec3 color;
uniform float time;
uniform float twinkleSpeed;
uniform float speed;
uniform float brightness;
uniform float distfading;
out vec4 oColor;

void main()
{
    vec2 uv = vUv + 0.5;
    uv.x += time * speed * 0.1;

    vec3 dir = vec3(uv * 2.0, 1.0);
    float s = 0.1;
    float fade = 0.01;
    vec3 starColor = vec3(0.0);

    for (int r = 0; r < 3; ++r)
    {
        vec3 p = (time * speed * twinkleSpeed) + dir * (s * 0.5);
        p = abs(vec3(1.3) - mod(p, vec3(2.6)));

        float prevLen = 0.0;
        float a = 0.0;
        for (int i = 0; i < 17; ++i)
        {
            p = abs(p);
            p = p * (1.0 / dot(p, p)) + vec3(-0.5);
            float len = length(p);
            a += abs(len - prevLen);
            prevLen = len;
        }

        a = a * a * a;
        starColor += (vec3(s, s * s, s * s * s) * a * brightness + 1.0) * fade;
        fade *= distfading;
        s += 0.2;
    }

    starColor = min(starColor, vec3(1.2));

    float intensity = min(starColor.r + starColor.g + starColor.b, 0.7);
    vec2 sgn = vUv * 2.0 - 1.0;
    vec2 gradient = vec2(dFdx(intensity) * sgn.x, dFdy(intensity) * sgn.y);
    float cutoff = max(max(gradient.x, gradient.y) - 0.1, 0.0);
    starColor *= max(1.0 - cutoff * 6.0, 0.3);

    oColor = vec4(starColor * color, 1.0);
}
)";

} // namespace

std::string ComplexShader::description() const
{
    return "Legacy 11_ComplexShader: ShaderFrog Universe Nursery starfield on "
           "a fullscreen quad.";
}

gpu::Status ComplexShader::setUp()
{
    const std::array<ComplexShaderVertex, 4u> corners{
        ComplexShaderVertex{ Vector2f(1.0f, 1.0f), Vector2f(1.0f, 1.0f) },
        ComplexShaderVertex{ Vector2f(1.0f, -1.0f), Vector2f(1.0f, 0.0f) },
        ComplexShaderVertex{ Vector2f(-1.0f, -1.0f), Vector2f(0.0f, 0.0f) },
        ComplexShaderVertex{ Vector2f(-1.0f, 1.0f), Vector2f(0.0f, 1.0f) } };
    const std::array<std::uint16_t, 6u> indices{ 0u, 1u, 3u, 1u, 2u, 3u };

    GPU_TRY_ASSIGN(vertices,
                   gpu::Buffer<ComplexShaderVertex>::from(
                       std::span<const ComplexShaderVertex>(corners),
                       gpu::BufferKind::Vertex,
                       gpu::BufferUsage::Immutable));
    m_vertices = std::move(vertices);
    GPU_TRY_ASSIGN(index_buffer,
                   gpu::Buffer<std::uint16_t>::from(
                       std::span<const std::uint16_t>(indices),
                       gpu::BufferKind::Index,
                       gpu::BufferUsage::Immutable));
    m_indices = std::move(index_buffer);

    GPU_TRY_ASSIGN(program, gpu::Program::fromSources(VERTEX, FRAGMENT));
    m_program = std::move(program);
    GPU_TRY(m_program.set("color", Vector3f(1.0f, 1.0f, 1.0f)));
    GPU_TRY(m_program.set("speed", 0.0001f));
    GPU_TRY(m_program.set("brightness", 0.0018f));
    GPU_TRY(m_program.set("distfading", 0.7f));
    GPU_TRY(m_program.set("twinkleSpeed", 200.0f));

    gpu::RenderState state;
    state.depth_test = false;
    GPU_TRY_ASSIGN(
        pipeline,
        gpu::Pipeline::create<ComplexShaderVertex>(
            m_program, GPU_LAYOUT(ComplexShaderVertex, position, uv), state));
    m_pipeline = std::move(pipeline);
    return gpu::success();
}

gpu::Status ComplexShader::draw(Frame const& p_frame)
{
    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = Vector4f(0.0f, 0.0f, 0.4f, 1.0f);
    desc.clear_depth = false;
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));
    GPU_TRY(m_program.set("time", p_frame.total));
    return gpu::drawIndexed(m_pipeline, m_vertices.handle(), m_indices.handle(),
                            gpu::IndexType::UInt16, 6u);
}

} // namespace examples
