#include "01_Scientific/30_Terrain3D.hpp"

#include "Common/DataPath.hpp"
#include "GPU/Draw.hpp"
#include "GPU/RenderPass.hpp"
#include "Math/Transformation.hpp"
#include "Math/Units.hpp"

#include <cmath>
#include <cstdlib>
#include <vector>

using namespace units::literals;

namespace examples
{

namespace
{

constexpr char const* VERTEX = R"(#version 450 core
in vec3 position;
in vec3 texCoord3D;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec3 vTexCoord3D;
void main()
{
    vTexCoord3D = texCoord3D;
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)";

constexpr char const* FRAGMENT = R"(#version 450 core
in vec3 vTexCoord3D;
uniform sampler3D tex3d;
out vec4 oColor;
void main()
{
    oColor = texture(tex3d, vTexCoord3D);
}
)";

std::vector<float> generateAltitudes(std::uint32_t p_dim)
{
    std::vector<float> altitudes(p_dim * p_dim);
    for (std::uint32_t i = 0u; i < p_dim * p_dim; ++i)
    {
        altitudes[i] = std::fabs(static_cast<float>(std::rand()) /
                                 static_cast<float>(RAND_MAX));
    }

    std::vector<float> smooth(p_dim * p_dim);
    for (std::uint32_t pass = 0u; pass < 5u; ++pass)
    {
        float max_val = 0.0f;
        float min_val = 1.0f;
        for (std::uint32_t x = 0u; x < p_dim; ++x)
        {
            for (std::uint32_t y = 0u; y < p_dim; ++y)
            {
                if ((x == 0u) || (x == p_dim - 1u) || (y == 0u) ||
                    (y == p_dim - 1u))
                {
                    altitudes[(x * p_dim) + y] = 0.0f;
                }
                else
                {
                    float sum = 0.0f;
                    std::uint32_t count = 0u;
                    for (std::uint32_t sx = 0u; sx <= 2u; ++sx)
                    {
                        for (std::uint32_t sy = 0u; sy <= 2u; ++sy)
                        {
                            sum += altitudes[((x + sx) - 1u) * p_dim +
                                             ((y + sy) - 1u)];
                            ++count;
                        }
                    }
                    const float value = sum / static_cast<float>(count);
                    smooth[(x * p_dim) + y] = value;
                    max_val = std::max(max_val, value);
                    min_val = std::min(min_val, value);
                }
            }
        }
        for (std::uint32_t i = 0u; i < p_dim * p_dim; ++i)
        {
            altitudes[i] = (smooth[i] - min_val) / (max_val - min_val);
        }
    }
    return altitudes;
}

gpu::Result<std::pair<gpu::Buffer<TerrainVertex>, gpu::Buffer<std::uint16_t>>>
buildTerrainMesh(std::uint32_t p_dim)
{
    const std::vector<float> altitudes = generateAltitudes(p_dim);
    constexpr float max_height = 0.2f;
    constexpr float tex_height = 0.9f;

    std::vector<TerrainVertex> corners;
    std::vector<std::uint16_t> indices;
    corners.reserve((p_dim - 1u) * (p_dim - 1u) * 4u);
    indices.reserve((p_dim - 1u) * (p_dim - 1u) * 6u);

    for (std::uint32_t x = 1u; x < p_dim; ++x)
    {
        for (std::uint32_t y = 1u; y < p_dim; ++y)
        {
            const auto sample = [&](std::uint32_t sx, std::uint32_t sy)
            {
                const float altitude = altitudes[(sx * p_dim) + sy];
                return TerrainVertex{
                    Vector3f((static_cast<float>(sx) /
                              static_cast<float>(p_dim)) -
                                 0.5f,
                             (static_cast<float>(sy) /
                              static_cast<float>(p_dim)) -
                                 0.5f,
                             altitude * max_height),
                    Vector3f(static_cast<float>(sx) /
                                 static_cast<float>(p_dim),
                             static_cast<float>(sy) /
                                 static_cast<float>(p_dim),
                             altitude * tex_height) };
            };

            const std::uint16_t base =
                static_cast<std::uint16_t>(corners.size());
            corners.push_back(sample(x - 1u, y - 1u));
            corners.push_back(sample(x, y - 1u));
            corners.push_back(sample(x - 1u, y));
            corners.push_back(sample(x, y));

            indices.push_back(base);
            indices.push_back(static_cast<std::uint16_t>(base + 1u));
            indices.push_back(static_cast<std::uint16_t>(base + 2u));
            indices.push_back(static_cast<std::uint16_t>(base + 1u));
            indices.push_back(static_cast<std::uint16_t>(base + 3u));
            indices.push_back(static_cast<std::uint16_t>(base + 2u));
        }
    }

    GPU_TRY_ASSIGN(vertices,
                   gpu::Buffer<TerrainVertex>::from(
                       std::span<const TerrainVertex>(corners),
                       gpu::BufferKind::Vertex,
                       gpu::BufferUsage::Immutable));
    GPU_TRY_ASSIGN(index_buffer,
                   gpu::Buffer<std::uint16_t>::from(
                       std::span<const std::uint16_t>(indices),
                       gpu::BufferKind::Index,
                       gpu::BufferUsage::Immutable));
    return std::pair<gpu::Buffer<TerrainVertex>, gpu::Buffer<std::uint16_t>>{
        std::move(vertices), std::move(index_buffer)
    };
}

} // namespace

std::string Terrain3D::description() const
{
    return "Legacy 08_TerrainTexture3D: a smoothed random height field samples "
           "six PNG layers stacked into a 3D texture.";
}

gpu::Status Terrain3D::setUp()
{
    std::srand(42);

    const std::array<char const*, 6u> layers{ "deep_water.png",
                                              "shallow_water.png",
                                              "shore.png",
                                              "fields.png",
                                              "rocks.png",
                                              "snow.png" };

    gpu::LoadOptions options;
    options.srgb = true;
    std::vector<std::byte> layer_bytes;
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    gpu::PixelFormat format = gpu::PixelFormat::RGBA8;

    for (char const* file : layers)
    {
        const std::string path = dataPath(file);
        if (path.empty())
        {
            return gpu::failure(
                "30_Terrain3D needs terrain PNGs in "
                "external/OpenGLCppWrapper-data/");
        }
        GPU_TRY_ASSIGN(image, gpu::Texture::fromFile(path, options));
        if (width == 0u)
        {
            width = image.width();
            height = image.height();
            format = image.format();
        }
        else if ((image.width() != width) || (image.height() != height))
        {
            return gpu::failure("terrain PNG layers must share the same size");
        }
        GPU_TRY_ASSIGN(pixels, image.read());
        layer_bytes.insert(layer_bytes.end(), pixels.begin(), pixels.end());
    }

    gpu::TextureDesc desc;
    desc.kind = gpu::TextureKind::Texture3D;
    desc.format = format;
    desc.width = width;
    desc.height = height;
    desc.depth = static_cast<std::uint32_t>(layers.size());
    desc.levels = 1u;
    desc.wrap_x = gpu::Wrap::ClampToEdge;
    desc.wrap_y = gpu::Wrap::ClampToEdge;
    desc.wrap_z = gpu::Wrap::ClampToEdge;
    GPU_TRY_ASSIGN(volume, gpu::Texture::create(desc));
    m_volume = std::move(volume);

    const std::size_t layer_bytes_count =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) *
        gpu::bytesPerPixel(format);
    for (std::uint32_t z = 0u; z < desc.depth; ++z)
    {
        GPU_TRY(m_volume.writeLayer(
            z,
            std::span<const std::byte>(layer_bytes.data() +
                                           (static_cast<std::size_t>(z) *
                                            layer_bytes_count),
                                       layer_bytes_count)));
    }

    GPU_TRY_ASSIGN(mesh, buildTerrainMesh(40u));
    m_vertices = std::move(mesh.first);
    m_indices = std::move(mesh.second);
    m_index_count = (40u - 1u) * (40u - 1u) * 6u;

    GPU_TRY_ASSIGN(program, gpu::Program::fromSources(VERTEX, FRAGMENT));
    m_program = std::move(program);
    gpu::RenderState state;
    state.depth_test = true;
    state.cull = gpu::CullMode::Back;
    GPU_TRY_ASSIGN(
        pipeline,
        gpu::Pipeline::create<TerrainVertex>(
            m_program,
            GPU_LAYOUT(TerrainVertex, position, texCoord3D),
            state));
    m_pipeline = std::move(pipeline);
    return gpu::success();
}

gpu::Status Terrain3D::draw(Frame const& p_frame)
{
    const float aspect =
        (p_frame.height == 0u)
            ? 1.0f
            : (static_cast<float>(p_frame.width) /
               static_cast<float>(p_frame.height));
    const Matrix44f projection = matrix::perspective(
        units::angle::degree_t(60.0), aspect, 0.1f, 10.0f);
    const Matrix44f view =
        matrix::lookAt(Vector3f(0.75f, -0.75f, 0.75f),
                       Vector3f(0.0f, 0.0f, 0.0f),
                       Vector3f(0.0f, 0.0f, 1.0f));
    const Matrix44f model(matrix::Identity);

    gpu::PassDesc pass_desc;
    pass_desc.width = p_frame.width;
    pass_desc.height = p_frame.height;
    pass_desc.color = Vector4f(0.0f, 0.0f, 0.4f, 1.0f);
    pass_desc.clear_depth = true;
    GPU_TRY_ASSIGN(pass, gpu::RenderPass::begin(pass_desc));
    GPU_TRY(m_volume.bind(0u));
    GPU_TRY(m_program.set("tex3d", 0));
    GPU_TRY(m_program.set("model", model));
    GPU_TRY(m_program.set("view", view));
    GPU_TRY(m_program.set("projection", projection));
    return gpu::drawIndexed(m_pipeline, m_vertices.handle(), m_indices.handle(),
                            gpu::IndexType::UInt16, m_index_count);
}

} // namespace examples
