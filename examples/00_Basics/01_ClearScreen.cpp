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

#include "00_Basics/01_ClearScreen.hpp"

#include <cmath>

namespace examples
{

//------------------------------------------------------------------------------
std::string ClearScreen::description() const
{
    return "A pass says where in the window a frame is drawn and what it starts "
           "from. There are two here, one per half of the window, which is why a "
           "pass takes a rectangle rather than being told to clear the screen. No "
           "buffer, no shader, no pipeline: the counters below stay at zero.";
}

//------------------------------------------------------------------------------
gpu::Status ClearScreen::setUp()
{
    // Nothing to build. Kept as a reminder that an example with nothing to set up
    // still says so, rather than leaving the reader to wonder where the setup went.
    return gpu::success();
}

//------------------------------------------------------------------------------
gpu::Status ClearScreen::draw(Frame const& p_frame)
{
    const float pulse =
        0.5f + (0.5f * std::sin(p_frame.total * 1.5f));

    {
        gpu::PassDesc left;
        left.width = p_frame.width / 2u;
        left.height = p_frame.height;
        left.color = Vector4f(0.1f, 0.1f + (0.6f * pulse), 0.3f, 1.0f);

        // Written out in full here, once, so that the GPU_TRY macro used from the
        // next example on is not mistaken for magic: a pass either opened or said
        // why not, and the reason is passed up rather than printed and forgotten.
        gpu::Result<gpu::RenderPass> pass = gpu::RenderPass::begin(left);
        if (!pass)
        {
            return gpu::failure(pass.error());
        }

        // The pass closes at the end of this scope, which is why there is a scope
        // here at all. Nothing has to be remembered, and a frame returning early
        // cannot leave one open.
    }

    {
        gpu::PassDesc right;
        right.x = p_frame.width / 2u;
        right.width = p_frame.width - right.x;
        right.height = p_frame.height;
        right.color = Vector4f(0.3f, 0.1f, 0.1f + (0.6f * (1.0f - pulse)), 1.0f);

        gpu::Result<gpu::RenderPass> pass = gpu::RenderPass::begin(right);
        if (!pass)
        {
            return gpu::failure(pass.error());
        }

        // The left half is untouched: this pass said where it draws, so what the
        // previous one left is still on the screen next to it.
    }

    return gpu::success();
}

} // namespace examples
