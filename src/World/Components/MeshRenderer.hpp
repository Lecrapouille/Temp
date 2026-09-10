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

#include "Assets/AssetIds.hpp"

#include <cstdint>

namespace world
{

// ****************************************************************************
//! \brief What can be done with a Renderable, packed as bit flags.
//!
//! Bits are stable identifiers, not indices into an array. The extractor reads
//! them to decide whether an item shows up in a given pass.
// ****************************************************************************
enum class RenderFlags : std::uint32_t
{
    //! \brief Default: shows up in every pass the material declares.
    None = 0u,
    //! \brief Ignored during the depth prepass. What a decal wants.
    NoDepthPrepass = 1u << 0u,
    //! \brief Ignored by shadow-caster passes.
    NoShadowCaster = 1u << 1u,
    //! \brief Ignored by shadow-receiver passes.
    NoShadowReceiver = 1u << 2u,
    //! \brief Drawn on top of everything at the end. What a UI billboard wants.
    Overlay = 1u << 3u,
};

[[nodiscard]] constexpr RenderFlags operator|(RenderFlags p_a, RenderFlags p_b)
{
    return static_cast<RenderFlags>(static_cast<std::uint32_t>(p_a) |
                                    static_cast<std::uint32_t>(p_b));
}

[[nodiscard]] constexpr bool has(RenderFlags p_flags, RenderFlags p_flag)
{
    return (static_cast<std::uint32_t>(p_flags) &
            static_cast<std::uint32_t>(p_flag)) != 0u;
}

// ****************************************************************************
//! \brief A declarative "draw this mesh with this material" attached to an
//! Entity.
//!
//! The component is data only. It holds ids into the AssetManager, not GPU
//! handles. The renderer resolves the ids to real \c gpu::Pipeline and
//! \c gpu::Buffer at extraction time.
//!
//! That is why an Entity of the World never touches OpenGL: the World can run
//! headless, be saved and reloaded, and the same MeshRenderer can be used with
//! a rebuilt device after a context loss without a single entity changing.
// ****************************************************************************
struct MeshRenderer
{
    //! \brief Which mesh to draw.
    assets::MeshAssetId mesh;
    //! \brief Which material instance to draw it with. The instance names the
    //! Material family, so the renderer picks the pipeline from there.
    assets::MaterialInstanceId material_instance;
    //! \brief Per-pass filtering. Default: shows up in every pass.
    RenderFlags flags = RenderFlags::None;
};

} // namespace world
