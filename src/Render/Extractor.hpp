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
#include "Render/RenderSnapshot.hpp"

namespace scene
{
class Scene;
}

namespace render
{

// ****************************************************************************
//! \brief Turns a Scene + World into a RenderSnapshot.
//!
//! Extraction is where the World stops and rendering starts. It reads:
//! - the Scene's active camera and settings;
//! - every MeshRenderer, DirectionalLight and PointLight in the World;
//! - every world matrix produced by the last \c World::update();
//! - the AssetManager, to skip items that refer to stale ids.
//!
//! It writes a RenderSnapshot: a value that stands on its own. The World may
//! be modified between an extraction and the frame that reads its snapshot;
//! the snapshot is what the frame draws.
//!
//! Frustum culling happens here too: an item whose world bounds fall entirely
//! outside the camera frustum is left out of the snapshot. That makes the
//! Renderer's job smaller and the culling itself easy to measure.
// ****************************************************************************
class Extractor
{
public:

    // ------------------------------------------------------------------------
    //! \brief Build a snapshot out of a Scene, using the given aspect ratio
    //! for the perspective projection.
    //!
    //! Requires the Scene to have a valid active camera Entity with a Camera
    //! component. Refuses otherwise, with a sentence: rendering with no
    //! camera should be an error, not a black screen.
    // ------------------------------------------------------------------------
    [[nodiscard]] static gloop::Result<RenderSnapshot>
    extract(scene::Scene const& p_scene, float p_aspect);

    //! \brief Same, but taking width and height instead of aspect. Convenience.
    [[nodiscard]] static gloop::Result<RenderSnapshot>
    extract(scene::Scene const& p_scene,
            std::uint32_t p_width,
            std::uint32_t p_height);
};

} // namespace render
