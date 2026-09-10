#pragma once

#include "Common/Example.hpp"
#include "GPU/Buffer.hpp"
#include "GPU/Pipeline.hpp"
#include "GPU/Shader.hpp"
#include "Math/Vector.hpp"

namespace examples
{

struct ComplexShaderVertex
{
    Vector2f position;
    Vector2f uv;
};

class ComplexShader: public Example
{
public:

    [[nodiscard]] std::string name() const override { return "31_ComplexShader"; }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    gpu::Program m_program;
    gpu::Pipeline m_pipeline;
    gpu::Buffer<ComplexShaderVertex> m_vertices;
    gpu::Buffer<std::uint16_t> m_indices;
};

} // namespace examples
