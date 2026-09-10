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

#include "GPU/Core/Result.hpp"
#include "GPU/Framebuffer.hpp"
#include "Math/Vector.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

// ****************************************************************************
//! \file
//! \brief Where a frame is drawn, and what it starts from.
//!
//! A pass is a span of drawing with one target, one viewport, and one decision
//! about what the target held before. Saying it out loud, once, at the top of the
//! frame, is what makes the rest of the frame readable: every draw inside a pass
//! goes to the same place, and a draw outside any pass is a mistake rather than a
//! draw into whatever happened to be bound.
//!
//! \code
//! auto pass = gpu::RenderPass::begin({ .width = width, .height = height,
//!                                      .clear_color = { 0.1f, 0.1f, 0.15f, 1.0f } });
//! if (!pass) { std::cerr << pass.error(); return; }
//!
//! GPU_TRY(gpu::draw(pipeline, mesh));
//! \endcode
//!
//! The pass ends when the object goes out of scope. There is nothing to remember
//! to call, and nothing that stays half open when a frame returns early.
//!
//! \note PassDesc::target chooses the window (empty) or a framebuffer. Drawing
//! into a texture is how 07_RenderToTexture and every ping-pong simulation work.
// ****************************************************************************

namespace gpu
{

// ****************************************************************************
//! \brief What a pass draws into and what it starts from.
// ****************************************************************************
struct PassDesc
{
    //! \brief Left edge of the part of the target being drawn into, in pixels.
    std::uint32_t x = 0u;
    //! \brief Bottom edge, in pixels. Zero is the bottom of the target, not the
    //! top, which is the one place this library keeps the convention of the
    //! graphics API rather than that of a window system.
    std::uint32_t y = 0u;
    //! \brief Width of the part being drawn into. Never zero: a pass drawing into
    //! nothing is a size that was never computed, most often a window whose size
    //! was read before it was mapped.
    std::uint32_t width = 0u;
    //! \brief Height of the part being drawn into.
    std::uint32_t height = 0u;

    //! \brief Should the target be filled with a colour before anything is drawn?
    //!
    //! Almost always yes. Leaving it off keeps whatever the previous frame drew,
    //! which is what an accumulating effect wants and what a forgotten clear looks
    //! like: a trail of smeared images.
    bool clear_color = true;
    //! \brief The colour to start from, as red, green, blue and alpha between 0
    //! and 1.
    Vector4f color{ 0.0f, 0.0f, 0.0f, 1.0f };

    //! \brief Should the recorded distances be forgotten before anything is
    //! drawn?
    //!
    //! Yes for the first pass of a frame. A second pass drawing on top of the
    //! first, such as an overlay that must respect the depth already there, says
    //! no.
    bool clear_depth = true;
    //! \brief The distance to start from. One is as far away as it gets, which is
    //! what a depth test using Less needs in order to let anything through.
    float depth = 1.0f;

    //! \brief Which target to draw into. Empty means the window. A handle that
    //! has been released is refused when the pass starts, rather than drawing
    //! into the window by accident.
    FramebufferHandle target{};
};

// ****************************************************************************
//! \brief One span of drawing, open until it goes out of scope.
// ****************************************************************************
class RenderPass
{
public:

    // ------------------------------------------------------------------------
    //! \brief Start drawing into the window.
    //!
    //! \param[in] p_desc where in the window, and what to start from.
    //! \return the open pass, or why it could not be started: a size of zero, or
    //! another pass already open.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<RenderPass> begin(PassDesc const& p_desc);

    RenderPass(RenderPass&& p_other) noexcept;
    RenderPass& operator=(RenderPass&& p_other) noexcept;
    RenderPass(RenderPass const&) = delete;
    RenderPass& operator=(RenderPass const&) = delete;

    // ------------------------------------------------------------------------
    //! \brief Close the pass.
    // ------------------------------------------------------------------------
    ~RenderPass();

    // ------------------------------------------------------------------------
    //! \brief Close the pass now rather than at the end of the scope.
    // ------------------------------------------------------------------------
    void end();

    // ------------------------------------------------------------------------
    //! \brief Is this pass still open?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool open() const
    {
        return m_open;
    }

    // ------------------------------------------------------------------------
    //! \brief Where in the target this pass draws, and what it started from.
    // ------------------------------------------------------------------------
    [[nodiscard]] PassDesc const& description() const
    {
        return m_desc;
    }

private:

    explicit RenderPass(PassDesc const& p_desc) : m_desc(p_desc), m_open(true) {}

    PassDesc m_desc;
    bool m_open = false;
};

// ----------------------------------------------------------------------------
//! \brief Is a pass open right now?
//!
//! What a draw call checks before agreeing to draw, since drawing outside a pass
//! goes somewhere nobody chose.
// ----------------------------------------------------------------------------
[[nodiscard]] bool inRenderPass();

// ----------------------------------------------------------------------------
//! \brief Where the open pass is drawing, or a description of zero size when
//! none is open.
// ----------------------------------------------------------------------------
[[nodiscard]] PassDesc const& currentPass();

// ----------------------------------------------------------------------------
//! \brief Read the picture back out of the target of the open pass.
//!
//! Four bytes per pixel, red, green, blue and alpha, with the bottom row first,
//! because that is the order the target holds them in and turning them over here
//! would only move the surprise elsewhere.
//!
//! This waits for the device to finish everything it was asked, so it belongs in
//! a screenshot, in a test, or in a tool, never in a frame.
//!
//! \param[in] p_x left edge of what to read, relative to the target.
//! \param[in] p_y bottom edge.
//! \param[in] p_width how many pixels across. Zero means the whole width of the
//! open pass.
//! \param[in] p_height how many pixels up. Zero means the whole height.
//! \return the pixels, or why they could not be read.
// ----------------------------------------------------------------------------
[[nodiscard]] Result<std::vector<std::byte>> readPixels(
    std::uint32_t p_x = 0u,
    std::uint32_t p_y = 0u,
    std::uint32_t p_width = 0u,
    std::uint32_t p_height = 0u);

} // namespace gpu
