//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#pragma once

#include "Compages/Core/Vector.hpp"

namespace scene
{

// ****************************************************************************
//! \brief Which projection a Camera component uses.
// ****************************************************************************
enum class Projection
{
    //! \brief Perspective projection with a vertical field of view.
    Perspective,
    //! \brief Orthographic projection with a symmetric half-height.
    Orthographic,
};

// ****************************************************************************
//! \brief How a Camera looks. Not where it sits.
//!
//! A Camera is a component attached to an EntityId of the World. The place from
//! which it looks is the EntityId's own transform, exactly like any other
//! spatial object: parent it to a rig, and it rides that rig. That is why the
//! Camera has no eye/target/up: those belong to a controller (an orbit
//! controller, a follow-target behaviour) which writes the transform.
//!
//! Perspective is what a game needs; orthographic is what a UI overlay or an
//! isometric view needs. The three parameters below cover both:
//! - \c fov_degrees is the vertical field of view for perspective, ignored
//!   otherwise;
//! - \c ortho_half_height is the half-height, in world units, of the
//!   orthographic view volume, ignored otherwise;
//! - \c near_plane and \c far_plane are the visible depth range, in both
//!   modes.
//! - \c viewport is the normalised rectangle of the target this camera
//!   draws into. The Extractor turns it into pixels on the CameraFrame.
// ****************************************************************************
// ****************************************************************************
//! \brief Where this camera draws, in normalised framebuffer coordinates.
//!
//! (0, 0, 1, 1) is the whole target. Split views set two cameras to
//! (0, 0, 0.5, 1) and (0.5, 0, 0.5, 1). The Extractor turns this into a
//! pixel rectangle on the \c CameraFrame.
// ****************************************************************************
struct Viewport
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 1.0f;
    float height = 1.0f;
};

struct Camera
{
    Projection projection = Projection::Perspective;
    float fov_degrees = 60.0f;
    float ortho_half_height = 1.0f;
    float near_plane = 0.1f;
    float far_plane = 1000.0f;
    Viewport viewport{};
};

} // namespace scene
