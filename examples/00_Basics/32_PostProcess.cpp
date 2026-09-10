#include "00_Basics/32_PostProcess.hpp"

#include "Common/DataPath.hpp"
#include "GPU/Draw.hpp"
#include "GPU/RenderPass.hpp"
#include "Math/Transformation.hpp"
#include "Math/Units.hpp"

#include <array>

using namespace units::literals;

namespace examples
{

namespace
{

constexpr char const* SCENE_VS = R"(#version 450 core
in vec3 position;
in vec2 uv;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec2 vUV;
void main()
{
    vUV = uv;
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)";

constexpr char const* SCENE_FS = R"(#version 450 core
in vec2 vUV;
uniform sampler2D texID;
out vec4 oColor;
void main()
{
    oColor = texture(texID, vUV);
}
)";

constexpr char const* SCREEN_VS = R"(#version 450 core
in vec2 position;
in vec2 uv;
out vec2 vUV;
void main()
{
    vUV = uv;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr char const* SCREEN_FS = R"(#version 450 core
in vec2 vUV;
uniform sampler2D texID;
uniform float time;
uniform float screen_width;
uniform float screen_height;
out vec4 oColor;
void main()
{
    oColor = vec4(texture(texID, vUV +
                          0.005 * vec2(sin(time + screen_width * vUV.x),
                                       cos(time + screen_height * vUV.y))).xyz,
                  1.0);
}
)";

gpu::Status loadTexture(char const* p_file, gpu::Texture& p_out)
{
    const std::string path = dataPath(p_file);
    if (path.empty())
    {
        return gpu::failure(std::string("missing texture: ") + p_file);
    }
    gpu::LoadOptions options;
    options.srgb = true;
    GPU_TRY_ASSIGN(tex, gpu::Texture::fromFile(path, options));
    p_out = std::move(tex);
    return gpu::success();
}

assets::MeshVertex meshVertex(Vector3f p_position, Vector2f p_uv)
{
    return assets::MeshVertex{ p_position, Vector3f(0.0f, 1.0f, 0.0f), p_uv };
}

} // namespace

std::string PostProcess::description() const
{
    return "Legacy 13_PostProdFrameBuffer: textured cube and floor rendered "
           "offscreen with depth, then a wavy fullscreen post pass.";
}

gpu::Status PostProcess::ensureTarget(std::uint32_t p_width,
                                      std::uint32_t p_height)
{
    if ((p_width == m_target_width) && (p_height == m_target_height) &&
        m_fbo.valid())
    {
        return gpu::success();
    }

    m_fbo.release();
    m_color_target.release();
    m_depth_target.release();

    gpu::TextureDesc color;
    color.kind = gpu::TextureKind::Texture2D;
    color.format = gpu::PixelFormat::RGBA8;
    color.width = p_width;
    color.height = p_height;
    color.levels = 1u;
    GPU_TRY_ASSIGN(colour_tex, gpu::Texture::create(color));
    m_color_target = std::move(colour_tex);

    gpu::TextureDesc depth;
    depth.kind = gpu::TextureKind::Texture2D;
    depth.format = gpu::PixelFormat::Depth32F;
    depth.width = p_width;
    depth.height = p_height;
    depth.levels = 1u;
    GPU_TRY_ASSIGN(depth_tex, gpu::Texture::create(depth));
    m_depth_target = std::move(depth_tex);

    GPU_TRY_ASSIGN(fbo_obj, gpu::Framebuffer::create(m_color_target, m_depth_target));
    m_fbo = std::move(fbo_obj);
    m_target_width = p_width;
    m_target_height = p_height;
    return gpu::success();
}

gpu::Status PostProcess::setUp()
{
    GPU_TRY(loadTexture("wooden-crate.jpg", m_crate_texture));
    GPU_TRY(loadTexture("path.png", m_floor_texture));

    const std::array<assets::MeshVertex, 36u> cube{
        meshVertex(Vector3f(-1.0f, -1.0f, -1.0f), Vector2f(0.0f, 0.0f)),
        meshVertex(Vector3f(1.0f, -1.0f, -1.0f), Vector2f(1.0f, 0.0f)),
        meshVertex(Vector3f(-1.0f, -1.0f, 1.0f), Vector2f(0.0f, 1.0f)),
        meshVertex(Vector3f(1.0f, -1.0f, -1.0f), Vector2f(1.0f, 0.0f)),
        meshVertex(Vector3f(1.0f, -1.0f, 1.0f), Vector2f(1.0f, 1.0f)),
        meshVertex(Vector3f(-1.0f, -1.0f, 1.0f), Vector2f(0.0f, 1.0f)),
        meshVertex(Vector3f(-1.0f, 1.0f, -1.0f), Vector2f(0.0f, 0.0f)),
        meshVertex(Vector3f(-1.0f, 1.0f, 1.0f), Vector2f(0.0f, 1.0f)),
        meshVertex(Vector3f(1.0f, 1.0f, -1.0f), Vector2f(1.0f, 0.0f)),
        meshVertex(Vector3f(1.0f, 1.0f, -1.0f), Vector2f(1.0f, 0.0f)),
        meshVertex(Vector3f(-1.0f, 1.0f, 1.0f), Vector2f(0.0f, 1.0f)),
        meshVertex(Vector3f(1.0f, 1.0f, 1.0f), Vector2f(1.0f, 1.0f)),
        meshVertex(Vector3f(-1.0f, -1.0f, 1.0f), Vector2f(1.0f, 0.0f)),
        meshVertex(Vector3f(1.0f, -1.0f, 1.0f), Vector2f(0.0f, 0.0f)),
        meshVertex(Vector3f(-1.0f, 1.0f, 1.0f), Vector2f(1.0f, 1.0f)),
        meshVertex(Vector3f(1.0f, -1.0f, 1.0f), Vector2f(0.0f, 0.0f)),
        meshVertex(Vector3f(1.0f, 1.0f, 1.0f), Vector2f(0.0f, 1.0f)),
        meshVertex(Vector3f(-1.0f, 1.0f, 1.0f), Vector2f(1.0f, 1.0f)),
        meshVertex(Vector3f(-1.0f, -1.0f, -1.0f), Vector2f(0.0f, 0.0f)),
        meshVertex(Vector3f(-1.0f, 1.0f, -1.0f), Vector2f(0.0f, 1.0f)),
        meshVertex(Vector3f(1.0f, -1.0f, -1.0f), Vector2f(1.0f, 0.0f)),
        meshVertex(Vector3f(1.0f, -1.0f, -1.0f), Vector2f(1.0f, 0.0f)),
        meshVertex(Vector3f(-1.0f, 1.0f, -1.0f), Vector2f(0.0f, 1.0f)),
        meshVertex(Vector3f(1.0f, 1.0f, -1.0f), Vector2f(1.0f, 1.0f)),
        meshVertex(Vector3f(-1.0f, -1.0f, 1.0f), Vector2f(0.0f, 0.0f)),
        meshVertex(Vector3f(-1.0f, 1.0f, -1.0f), Vector2f(0.0f, 1.0f)),
        meshVertex(Vector3f(-1.0f, -1.0f, -1.0f), Vector2f(1.0f, 0.0f)),
        meshVertex(Vector3f(-1.0f, -1.0f, 1.0f), Vector2f(0.0f, 0.0f)),
        meshVertex(Vector3f(-1.0f, 1.0f, 1.0f), Vector2f(0.0f, 1.0f)),
        meshVertex(Vector3f(-1.0f, 1.0f, -1.0f), Vector2f(1.0f, 1.0f)),
        meshVertex(Vector3f(1.0f, -1.0f, -1.0f), Vector2f(0.0f, 0.0f)),
        meshVertex(Vector3f(1.0f, 1.0f, -1.0f), Vector2f(0.0f, 1.0f)),
        meshVertex(Vector3f(1.0f, -1.0f, 1.0f), Vector2f(1.0f, 0.0f)),
        meshVertex(Vector3f(1.0f, -1.0f, 1.0f), Vector2f(1.0f, 0.0f)),
        meshVertex(Vector3f(1.0f, 1.0f, -1.0f), Vector2f(0.0f, 1.0f)),
        meshVertex(Vector3f(1.0f, 1.0f, 1.0f), Vector2f(1.0f, 1.0f)) };

    const std::array<assets::MeshVertex, 6u> floor{
        meshVertex(Vector3f(5.0f, -1.5f, 5.0f), Vector2f(0.0f, 0.0f)),
        meshVertex(Vector3f(-5.0f, -1.5f, 5.0f), Vector2f(1.0f, 0.0f)),
        meshVertex(Vector3f(-5.0f, -1.5f, -5.0f), Vector2f(0.0f, 1.0f)),
        meshVertex(Vector3f(5.0f, -1.5f, 5.0f), Vector2f(1.0f, 0.0f)),
        meshVertex(Vector3f(-5.0f, -1.5f, -5.0f), Vector2f(1.0f, 1.0f)),
        meshVertex(Vector3f(5.0f, -1.5f, -5.0f), Vector2f(0.0f, 1.0f)) };

    const std::array<ScreenVertex, 6u> screen{
        ScreenVertex{ Vector2f(-1.0f, 1.0f), Vector2f(0.0f, 1.0f) },
        ScreenVertex{ Vector2f(-1.0f, -1.0f), Vector2f(0.0f, 0.0f) },
        ScreenVertex{ Vector2f(1.0f, -1.0f), Vector2f(1.0f, 0.0f) },
        ScreenVertex{ Vector2f(-1.0f, 1.0f), Vector2f(0.0f, 1.0f) },
        ScreenVertex{ Vector2f(1.0f, -1.0f), Vector2f(1.0f, 0.0f) },
        ScreenVertex{ Vector2f(1.0f, 1.0f), Vector2f(1.0f, 1.0f) } };

    const std::array<std::uint16_t, 6u> screen_indices{ 0u, 1u, 2u, 3u, 4u, 5u };

    GPU_TRY_ASSIGN(cube_buffer,
                   gpu::Buffer<assets::MeshVertex>::from(
                       std::span<const assets::MeshVertex>(cube),
                       gpu::BufferKind::Vertex,
                       gpu::BufferUsage::Immutable));
    m_cube_vertices = std::move(cube_buffer);
    GPU_TRY_ASSIGN(floor_buffer,
                   gpu::Buffer<assets::MeshVertex>::from(
                       std::span<const assets::MeshVertex>(floor),
                       gpu::BufferKind::Vertex,
                       gpu::BufferUsage::Immutable));
    m_floor_vertices = std::move(floor_buffer);
    GPU_TRY_ASSIGN(screen_buffer,
                   gpu::Buffer<ScreenVertex>::from(
                       std::span<const ScreenVertex>(screen),
                       gpu::BufferKind::Vertex,
                       gpu::BufferUsage::Immutable));
    m_screen_vertices = std::move(screen_buffer);
    GPU_TRY_ASSIGN(screen_index_buffer,
                   gpu::Buffer<std::uint16_t>::from(
                       std::span<const std::uint16_t>(screen_indices),
                       gpu::BufferKind::Index,
                       gpu::BufferUsage::Immutable));
    m_screen_indices = std::move(screen_index_buffer);

    GPU_TRY_ASSIGN(scene_program,
                   gpu::Program::fromSources(SCENE_VS, SCENE_FS));
    m_scene_program = std::move(scene_program);
    gpu::RenderState scene_state;
    scene_state.depth_test = true;
    scene_state.cull = gpu::CullMode::Back;
    GPU_TRY_ASSIGN(
        scene_pipeline,
        gpu::Pipeline::create<assets::MeshVertex>(
            m_scene_program,
            GPU_LAYOUT(assets::MeshVertex, position, normal, uv),
            scene_state));
    m_scene_pipeline = std::move(scene_pipeline);

    GPU_TRY_ASSIGN(screen_program,
                   gpu::Program::fromSources(SCREEN_VS, SCREEN_FS));
    m_screen_program = std::move(screen_program);
    gpu::RenderState screen_state;
    screen_state.depth_test = false;
    GPU_TRY_ASSIGN(
        screen_pipeline,
        gpu::Pipeline::create<ScreenVertex>(
            m_screen_program,
            GPU_LAYOUT(ScreenVertex, position, uv),
            screen_state));
    m_screen_pipeline = std::move(screen_pipeline);

    return gpu::success();
}

gpu::Status PostProcess::draw(Frame const& p_frame)
{
    GPU_TRY(ensureTarget(p_frame.width, p_frame.height));

    const float aspect =
        (p_frame.height == 0u)
            ? 1.0f
            : (static_cast<float>(p_frame.width) /
               static_cast<float>(p_frame.height));
    const Matrix44f projection = matrix::perspective(
        units::angle::degree_t(50.0), aspect, 0.1f, 10.0f);
    const Matrix44f view =
        matrix::lookAt(Vector3f(3.0f, 3.0f, 3.0f), Vector3f(0.0f, 0.0f, 0.0f),
                       Vector3f(0.0f, 1.0f, 0.0f));
    const Matrix44f cube_model =
        matrix::rotate(Matrix44f(matrix::Identity),
                       units::angle::radian_t(p_frame.total * 0.7f),
                       Vector3f(0.0f, 1.0f, 0.0f));
    const Matrix44f floor_model(matrix::Identity);

    GPU_TRY(m_scene_program.set("view", view));
    GPU_TRY(m_scene_program.set("projection", projection));

    {
        gpu::PassDesc offscreen;
        offscreen.width = p_frame.width;
        offscreen.height = p_frame.height;
        offscreen.target = m_fbo.handle();
        offscreen.color = Vector4f(0.0f, 0.0f, 0.4f, 1.0f);
        offscreen.clear_depth = true;
        GPU_TRY_ASSIGN(offscreen_pass, gpu::RenderPass::begin(offscreen));

        GPU_TRY(m_floor_texture.bind(0u));
        GPU_TRY(m_scene_program.set("texID", 0));
        GPU_TRY(m_scene_program.set("model", floor_model));
        GPU_TRY(gpu::draw(m_scene_pipeline, m_floor_vertices.handle(), 6u));

        GPU_TRY(m_crate_texture.bind(0u));
        GPU_TRY(m_scene_program.set("model", cube_model));
        GPU_TRY(gpu::draw(m_scene_pipeline, m_cube_vertices.handle(), 36u));
    }

    gpu::PassDesc screen;
    screen.width = p_frame.width;
    screen.height = p_frame.height;
    screen.color = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
    screen.clear_depth = false;
    GPU_TRY_ASSIGN(screen_pass, gpu::RenderPass::begin(screen));
    GPU_TRY(m_color_target.bind(0u));
    GPU_TRY(m_screen_program.set("texID", 0));
    GPU_TRY(m_screen_program.set("time", p_frame.total));
    GPU_TRY(m_screen_program.set("screen_width",
                                 static_cast<float>(p_frame.width)));
    GPU_TRY(m_screen_program.set("screen_height",
                                 static_cast<float>(p_frame.height)));
    return gpu::drawIndexed(m_screen_pipeline, m_screen_vertices.handle(),
                            m_screen_indices.handle(), gpu::IndexType::UInt16,
                            6u);
}

} // namespace examples
