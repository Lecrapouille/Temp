#pragma once

#include "Common/Example.hpp"
#include "GPU/Buffer.hpp"
#include "GPU/Pipeline.hpp"
#include "GPU/Shader.hpp"
#include "GPU/Texture.hpp"
#include "Math/Vector.hpp"

namespace examples
{

struct TerrainVertex
{
    Vector3f position;
    Vector3f texCoord3D;
};

class Terrain3D: public Example
{
public:

    [[nodiscard]] std::string name() const override { return "30_Terrain3D"; }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    gpu::Program m_program;
    gpu::Pipeline m_pipeline;
    gpu::Texture m_volume;
    gpu::Buffer<TerrainVertex> m_vertices;
    gpu::Buffer<std::uint16_t> m_indices;
    std::size_t m_index_count = 0u;
};

} // namespace examples
