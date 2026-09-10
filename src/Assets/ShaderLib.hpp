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

namespace assets::shaders
{

// ****************************************************************************
//! \file
//! \brief GLSL sources grouped by feature, not by backend.
//!
//! ShaderLib only returns source strings. Primitives and loaders turn them
//! into \c gpu::Program and \c gpu::Pipeline through the AssetManager.
// ****************************************************************************

inline constexpr int MAX_POINT_LIGHTS = 4;

[[nodiscard]] char const* litVertex();
[[nodiscard]] char const* litFragment();

[[nodiscard]] char const* pbrVertex();
[[nodiscard]] char const* pbrFragment();

[[nodiscard]] char const* depthVertex();
[[nodiscard]] char const* depthFragment();

[[nodiscard]] char const* normalsVertex();
[[nodiscard]] char const* normalsFragment();

} // namespace assets::shaders
