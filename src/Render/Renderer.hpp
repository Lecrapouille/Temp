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

#include "Common/Result.hpp"
#include "Render/RenderQueue.hpp"
#include "Render/RenderSnapshot.hpp"

namespace assets
{
class AssetManager;
}

namespace gpu
{
class RenderPass;
}

namespace render
{

// ****************************************************************************
//! \brief Consumes a RenderSnapshot and produces \c gpu:: calls.
//!
//! The Renderer is stateless in the sense that it does not own snapshots,
//! queues or camera frames: each call takes them as arguments. It owns a
//! scratch \c RenderQueue that is reused frame after frame, so a per-frame
//! allocation is avoided.
//!
//! It does not open the pass. Whoever prepares the frame opens the pass; the
//! Renderer draws into it. That way an application can compose several draws
//! into one pass (main + overlay, main + gizmos) without the Renderer needing
//! to know.
// ****************************************************************************
class Renderer
{
public:

    Renderer() = default;
    Renderer(Renderer const&) = delete;
    Renderer& operator=(Renderer const&) = delete;

    // ------------------------------------------------------------------------
    //! \brief Draw the snapshot into the open pass.
    //!
    //! \param[in] p_pass the open pass. Only checked for its existence: what
    //! the pass draws into is the pass's own concern.
    //! \param[in] p_snapshot what to draw.
    //! \param[in,out] p_assets the AssetManager to resolve ids against. Must
    //! be the one the snapshot was extracted from. Taken by non-const
    //! reference because \c gpu::Program::set has side-effects on the GPU
    //! side that require a mutable Program.
    // ------------------------------------------------------------------------
    [[nodiscard]] gloop::Status render(gpu::RenderPass const& p_pass,
                                     RenderSnapshot const& p_snapshot,
                                     assets::AssetManager& p_assets);

    //! \brief The queue that was built during the last render, for inspection.
    [[nodiscard]] RenderQueue const& queue() const { return m_queue; }

private:

    RenderQueue m_queue;
};

} // namespace render
