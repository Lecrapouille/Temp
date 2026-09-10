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

#include "GPU/RenderPass.hpp"
#include "GPU/Backends/Backend.hpp"
#include "GPU/Device.hpp"
#include "GPU/Internal/Pools.hpp"
#include "GPU/Internal/Statistics.hpp"

namespace gpu
{

namespace
{

//! \brief What the open pass is drawing, or a description of zero size.
//!
//! One pass at a time, and the library knows which. That is what lets a draw call
//! say "no pass is open" rather than drawing into whatever the driver happened to
//! have bound, which is how a frame ends up in the wrong place with no error at
//! all.
PassDesc g_current;
bool g_open = false;

} // namespace

//------------------------------------------------------------------------------
Result<RenderPass> RenderPass::begin(PassDesc const& p_desc)
{
    if (!initialized())
    {
        return failure("gpu::init() has not been called, so there is nothing to "
                       "draw into");
    }
    if (g_open)
    {
        return failure(
            "a pass is already open. Passes do not nest: let the first one go out "
            "of scope, or call end() on it, before starting another");
    }
    if ((p_desc.width == 0u) || (p_desc.height == 0u))
    {
        return failure(
            "a pass of " + std::to_string(p_desc.width) + " by " +
            std::to_string(p_desc.height) +
            " pixels draws nothing at all. A size of zero usually means the "
            "window size was read before the window was shown, or that the window "
            "is minimised, in which case the frame is worth skipping entirely");
    }

    backend::NativeId target = 0u;
    if (p_desc.target)
    {
        detail::FramebufferRecord const* record =
            detail::pools().framebuffers.get(p_desc.target);
        if (record == nullptr)
        {
            return failure(
                "the pass names a framebuffer that has been released, so there "
                "is nowhere to draw. The empty handle is the window; a stale "
                "one is not");
        }
        if ((p_desc.x + p_desc.width > record->width) ||
            (p_desc.y + p_desc.height > record->height))
        {
            return failure(
                "the pass is " + std::to_string(p_desc.width) + " by " +
                std::to_string(p_desc.height) + " at " +
                std::to_string(p_desc.x) + ", " + std::to_string(p_desc.y) +
                " and the framebuffer is " + std::to_string(record->width) +
                " by " + std::to_string(record->height));
        }
        target = record->native;
    }

    backend::beginPass(p_desc, target);

    g_current = p_desc;
    g_open = true;
    detail::countPass();

    return RenderPass(p_desc);
}

//------------------------------------------------------------------------------
RenderPass::RenderPass(RenderPass&& p_other) noexcept
    : m_desc(p_other.m_desc), m_open(p_other.m_open)
{
    p_other.m_open = false;
}

//------------------------------------------------------------------------------
RenderPass& RenderPass::operator=(RenderPass&& p_other) noexcept
{
    if (this != &p_other)
    {
        end();
        m_desc = p_other.m_desc;
        m_open = p_other.m_open;
        p_other.m_open = false;
    }
    return *this;
}

//------------------------------------------------------------------------------
RenderPass::~RenderPass()
{
    end();
}

//------------------------------------------------------------------------------
void RenderPass::end()
{
    if (!m_open)
    {
        return;
    }
    m_open = false;

    if (initialized())
    {
        backend::endPass();
    }
    g_open = false;
}

//------------------------------------------------------------------------------
bool inRenderPass()
{
    return g_open;
}

//------------------------------------------------------------------------------
PassDesc const& currentPass()
{
    static const PassDesc nothing;
    return g_open ? g_current : nothing;
}

//------------------------------------------------------------------------------
Result<std::vector<std::byte>> readPixels(std::uint32_t p_x,
                                          std::uint32_t p_y,
                                          std::uint32_t p_width,
                                          std::uint32_t p_height)
{
    if (!g_open)
    {
        return failure(
            "there is no pass open, so there is no target to read from. Read the "
            "picture before the pass goes out of scope");
    }

    const std::uint32_t width = (p_width == 0u) ? g_current.width : p_width;
    const std::uint32_t height = (p_height == 0u) ? g_current.height : p_height;

    if ((p_x + width > g_current.width) || (p_y + height > g_current.height))
    {
        return failure("the region being read runs outside the pass: " +
                       std::to_string(width) + " by " + std::to_string(height) +
                       " at " + std::to_string(p_x) + ", " + std::to_string(p_y) +
                       " does not fit in " + std::to_string(g_current.width) +
                       " by " + std::to_string(g_current.height));
    }

    std::vector<std::byte> pixels(static_cast<std::size_t>(width) * height * 4u);
    backend::readTargetPixels(
        g_current.x + p_x, g_current.y + p_y, width, height, pixels.data());
    return pixels;
}

} // namespace gpu
