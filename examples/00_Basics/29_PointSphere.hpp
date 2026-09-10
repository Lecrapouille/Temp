#pragma once

#include "Common/Example.hpp"
#include "GPU/Pipeline.hpp"
#include "GPU/Shader.hpp"
#include "GPU/Buffer.hpp"
#include "Math/Vector.hpp"

namespace examples
{

struct PointVertex
{
    Vector3f position;
};

class PointSphere: public Example
{
public:

    [[nodiscard]] std::string name() const override { return "29_PointSphere"; }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    gpu::Program m_program;
    gpu::Pipeline m_pipeline;
    gpu::Buffer<PointVertex> m_points;
    std::size_t m_point_count = 0u;
};

} // namespace examples
