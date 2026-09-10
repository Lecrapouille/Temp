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

#include "Assets/Material.hpp"
#include "Assets/MeshAsset.hpp"
#include "Common/Result.hpp"

#include <cstdint>

namespace assets
{

// ****************************************************************************
//! \file
//! \brief Helpers that build the meshes and materials the examples need,
//! without a loader.
//!
//! These are what a Three.js user would call \c BoxGeometry, \c SphereGeometry
//! and \c MeshLambertMaterial. They live at the assets level, not in the World,
//! because a mesh is data on the device and does not know which entity draws
//! it.
// ****************************************************************************

[[nodiscard]] gloop::Result<MeshAsset> makeCube();

[[nodiscard]] gloop::Result<MeshAsset> makeBox(float p_width,
                                             float p_height,
                                             float p_depth);

[[nodiscard]] gloop::Result<MeshAsset> makeSphere(float p_radius = 0.5f,
                                                std::uint32_t p_stacks = 16u,
                                                std::uint32_t p_slices = 24u);

[[nodiscard]] gloop::Result<MeshAsset> makePlane(float p_width = 1.0f,
                                               float p_height = 1.0f,
                                               std::uint32_t p_x_segments = 1u,
                                               std::uint32_t p_y_segments = 1u);

//! \brief Legacy Tube: frustum between two radii, centred on the origin.
[[nodiscard]] gloop::Result<MeshAsset> makeTube(float p_top_radius,
                                              float p_bottom_radius,
                                              float p_height,
                                              std::uint32_t p_slices = 16u,
                                              bool p_tip_along_negative_z = false);

//! \brief Cone with tip on \c -Z so \c world::lookAt() aims it at the target.
[[nodiscard]] gloop::Result<MeshAsset> makeCone(float p_bottom_radius,
                                              float p_top_radius,
                                              float p_height,
                                              std::uint32_t p_slices = 16u);

[[nodiscard]] gloop::Result<MeshAsset> makeCylinder(float p_radius,
                                                    float p_height,
                                                    std::uint32_t p_slices = 16u);

[[nodiscard]] gloop::Result<MeshAsset> makePyramid(float p_radius,
                                                   float p_height);

[[nodiscard]] gloop::Result<Material> makeLitMaterial();

[[nodiscard]] gloop::Result<Material> makePbrMaterial();

//! \brief Legacy DepthMaterial: eye-space depth as greyscale.
[[nodiscard]] gloop::Result<Material> makeDepthMaterial();

//! \brief Legacy NormalsMaterial: encoded surface normal as RGB.
[[nodiscard]] gloop::Result<Material> makeNormalsMaterial();

} // namespace assets
