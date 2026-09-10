#pragma once

#include "Common/Example.hpp"
#include "Assets/MeshAsset.hpp"
#include "GPU/Buffer.hpp"
#include "GPU/Framebuffer.hpp"
#include "GPU/Pipeline.hpp"
#include "GPU/Shader.hpp"
#include "GPU/Texture.hpp"
#include "Math/Vector.hpp"

namespace examples
{

struct ScreenVertex
{
    Vector2f position;
    Vector2f uv;
};

class PostProcess: public Example
{
public:

    [[nodiscard]] std::string name() const override { return "32_PostProcess"; }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    [[nodiscard]] gpu::Status ensureTarget(std::uint32_t p_width,
                                           std::uint32_t p_height);

    gpu::Program m_scene_program;
    gpu::Pipeline m_scene_pipeline;
    gpu::Program m_screen_program;
    gpu::Pipeline m_screen_pipeline;
    gpu::Framebuffer m_fbo;
    gpu::Texture m_color_target;
    gpu::Texture m_depth_target;
    gpu::Texture m_crate_texture;
    gpu::Texture m_floor_texture;
    gpu::Buffer<assets::MeshVertex> m_cube_vertices;
    gpu::Buffer<std::uint16_t> m_cube_indices;
    gpu::Buffer<assets::MeshVertex> m_floor_vertices;
    gpu::Buffer<ScreenVertex> m_screen_vertices;
    gpu::Buffer<std::uint16_t> m_screen_indices;
    std::uint32_t m_target_width = 0u;
    std::uint32_t m_target_height = 0u;
};

} // namespace examples
