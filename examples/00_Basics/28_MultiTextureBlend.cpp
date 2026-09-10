#include "00_Basics/28_MultiTextureBlend.hpp"

#include "Assets/Primitives.hpp"
#include "Common/DataPath.hpp"
#include "GPU/Draw.hpp"
#include "GPU/RenderPass.hpp"

namespace examples
{

namespace
{

constexpr char const* VERTEX = R"(#version 450 core
in vec3 position;
in vec2 uv;
out vec2 vUV;
void main()
{
    vUV = uv * 4.0;
    gl_Position = vec4(position.xy, 0.0, 1.0);
}
)";

constexpr char const* FRAGMENT = R"(#version 450 core
in vec2 vUV;
uniform sampler2D backgroundTexture;
uniform sampler2D rTexture;
uniform sampler2D gTexture;
uniform sampler2D bTexture;
uniform sampler2D blendMap;
out vec4 oColor;
void main()
{
    vec4 blend = texture(blendMap, vUV);
    float backAmount = 1.0 - (blend.r + blend.g + blend.b);
    vec4 colour = texture(backgroundTexture, vUV) * backAmount;
    colour += texture(rTexture, vUV) * blend.r;
    colour += texture(gTexture, vUV) * blend.g;
    colour += texture(bTexture, vUV) * blend.b;
    oColor = colour;
}
)";

gpu::Status loadTexture(std::string const& p_path, gpu::Texture& p_out)
{
    gpu::LoadOptions options;
    options.srgb = true;
    GPU_TRY_ASSIGN(tex, gpu::Texture::fromFile(p_path, options));
    p_out = std::move(tex);
    return gpu::success();
}

} // namespace

std::string MultiTextureBlend::description() const
{
    return "Legacy 03_MultiTexturedSquare: a blend map mixes four terrain "
           "materials on a subdivided plane. Textures come from "
           "OpenGLCppWrapper-data.";
}

gpu::Status MultiTextureBlend::setUp()
{
    GPU_TRY_ASSIGN(mesh, assets::makePlane(2.0f, 2.0f, 1u, 1u));
    m_vertices = std::move(mesh.vertices);
    m_indices = std::move(mesh.indices);
    m_index_count = mesh.index_count;

    GPU_TRY_ASSIGN(program, gpu::Program::fromSources(VERTEX, FRAGMENT));
    m_program = std::move(program);
    const gpu::VertexLayout layout =
        GPU_LAYOUT(assets::MeshVertex, position, uv);
    gpu::RenderState state;
    state.cull = gpu::CullMode::None;
    GPU_TRY_ASSIGN(pipeline,
                   gpu::Pipeline::create<assets::MeshVertex>(m_program, layout,
                                                             state));
    m_pipeline = std::move(pipeline);

    const std::array<std::pair<char const*, gpu::Texture*>, 5u> maps{ {
        { "blendMap.png", &m_blend_map },
        { "grassy2.png", &m_background },
        { "mud.png", &m_red },
        { "grassFlowers.png", &m_green },
        { "path.png", &m_blue },
    } };
    for (auto const& [file, slot] : maps)
    {
        const std::string path = dataPath(file);
        if (path.empty())
        {
            return gpu::failure(
                "28_MultiTextureBlend needs textures in "
                "external/OpenGLCppWrapper-data/");
        }
        GPU_TRY(loadTexture(path, *slot));
    }
    return gpu::success();
}

gpu::Status MultiTextureBlend::draw(Frame const& p_frame)
{
    gpu::PassDesc desc;
    desc.width = p_frame.width;
    desc.height = p_frame.height;
    desc.color = Vector4f(0.08f, 0.10f, 0.12f, 1.0f);
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(desc));

    GPU_TRY(m_blend_map.bind(0u));
    GPU_TRY(m_background.bind(1u));
    GPU_TRY(m_red.bind(2u));
    GPU_TRY(m_green.bind(3u));
    GPU_TRY(m_blue.bind(4u));
    GPU_TRY(m_program.set("blendMap", 0));
    GPU_TRY(m_program.set("backgroundTexture", 1));
    GPU_TRY(m_program.set("rTexture", 2));
    GPU_TRY(m_program.set("gTexture", 3));
    GPU_TRY(m_program.set("bTexture", 4));

    return gpu::drawIndexed(m_pipeline, m_vertices.handle(), m_indices.handle(),
                            gpu::IndexType::UInt16,
                            m_index_count);
}

} // namespace examples
