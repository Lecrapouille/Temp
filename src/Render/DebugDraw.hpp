//=============================================================================
// OpenGLCppWrapper: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenGLCppWrapper is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#pragma once

#include "GPU/Buffer.hpp"
#include "Common/Result.hpp"
#include "GPU/Pipeline.hpp"
#include "GPU/Shader.hpp"
#include "Math/AABB.hpp"
#include "Math/Matrix.hpp"
#include "Math/Vector.hpp"
#include "Render/CameraFrame.hpp"

#include <vector>

namespace render
{

// ****************************************************************************
//! \brief Immediate-mode debug overlay: lines and wire boxes.
//!
//! Call \c clear() at the start of a frame, queue primitives, then \c flush()
//! after the main Renderer while the same pass is open.
// ****************************************************************************
class DebugDraw
{
public:

    [[nodiscard]] gloop::Status ensureInitialized();

    void clear();

    void line(Vector3f const& p_a,
              Vector3f const& p_b,
              Vector3f const& p_color = Vector3f(1.0f, 0.2f, 0.2f));

    void box(AABB const& p_local_bounds,
             Matrix44f const& p_world_matrix,
             Vector3f const& p_color = Vector3f(0.2f, 1.0f, 0.4f));

    void ray(Vector3f const& p_origin,
             Vector3f const& p_direction,
             float p_length,
             Vector3f const& p_color = Vector3f(1.0f, 1.0f, 0.2f));

    [[nodiscard]] gloop::Status flush(CameraFrame const& p_camera);

private:

    struct DebugVertex
    {
        Vector3f position;
        Vector3f color;
    };

    bool m_ready = false;
    gpu::Program m_program;
    gpu::Pipeline m_pipeline;
    gpu::Buffer<DebugVertex> m_vertices;
    std::vector<DebugVertex> m_pending;
};

} // namespace render
