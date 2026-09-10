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

#include "Common/Example.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A picture drawn into textures, then read as a picture.
//!
//! The window is one target. A framebuffer is another: colour and depth textures
//! wired together so that a pass writes into them instead of onto the screen.
//! The cube of 05 is drawn only there. The window never sees it as a mesh; it
//! sees the texture that pass produced, once as it is and once through a
//! fullscreen effect.
//!
//! Two things that used to be easy to get wrong are said out loud. The textures
//! are named, not owned: destroying the framebuffer does not destroy the images,
//! which is why the second pass can still sample them after the first has
//! closed. And the offscreen picture is a size of its own, not the size of the
//! window, so a resize does not force the scene to be drawn again at a new
//! resolution just because the window changed.
// ****************************************************************************
class RenderToTexture: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "07_RenderToTexture";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] gpu::Status setUp() override;
    [[nodiscard]] gpu::Status draw(Frame const& p_frame) override;

private:

    struct Vertex
    {
        Vector3f position;
        Vector3f normal;
        Vector3f color;
    };

    [[nodiscard]] gpu::Status makeScene();
    [[nodiscard]] gpu::Status makeTarget();
    [[nodiscard]] gpu::Status makeScreen(gpu::Program& p_program,
                                         gpu::Pipeline& p_pipeline,
                                         char const* p_fragment);

    gpu::Program m_scene_program;
    gpu::Pipeline m_scene;
    gpu::Buffer<Vertex> m_vertices;
    gpu::Buffer<std::uint16_t> m_indices;

    gpu::Texture m_color;
    gpu::Texture m_depth;
    gpu::Framebuffer m_target;

    gpu::Program m_blit_program;
    gpu::Pipeline m_blit;
    gpu::Program m_process_program;
    gpu::Pipeline m_process;
};

} // namespace examples
