#pragma once

#include "Common/Example.hpp"
#include "Assets/MeshAsset.hpp"
#include "GPU/Pipeline.hpp"
#include "GPU/Shader.hpp"
#include "GPU/Texture.hpp"

namespace examples
{

class MultiTextureBlend: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "28_MultiTextureBlend";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    gpu::Program m_program;
    gpu::Pipeline m_pipeline;
    gpu::Buffer<assets::MeshVertex> m_vertices;
    gpu::Buffer<std::uint16_t> m_indices;
    std::size_t m_index_count = 0u;
    gpu::Texture m_blend_map;
    gpu::Texture m_background;
    gpu::Texture m_red;
    gpu::Texture m_green;
    gpu::Texture m_blue;
};

} // namespace examples
