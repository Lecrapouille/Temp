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

#include "GPU/Texture.hpp"

#include <string>

namespace assets
{

// ****************************************************************************
//! \brief A texture owned by the AssetManager.
//!
//! World and Scene never hold a \c gpu::Texture directly: they store a
//! TextureAssetId and ask the manager when a draw needs a binding.
// ****************************************************************************
struct TextureAsset
{
    //! \brief A short label, used in logs and when deduplicating imports.
    std::string name;
    //! \brief The device image.
    gpu::Texture texture;
};

} // namespace assets
