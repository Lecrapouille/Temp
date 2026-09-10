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

#include "Math/Frustum.hpp"
#include "Math/Matrix.hpp"
#include "Math/Ray.hpp"
#include "Math/Vector.hpp"

#include <cstdint>

namespace render
{

// ****************************************************************************
//! \brief Everything one frame of rendering needs from a Camera. Immutable.
//!
//! A CameraFrame is computed once per frame from a Camera component and the
//! World matrix of its Entity, and handed to the renderer. Objects never ask
//! the Camera what it is doing: they receive this.
//!
//! The inverses are stored so a picker can unproject a pixel without walking
//! back to the World. \c screenRay() is the only operation that needs them.
// ****************************************************************************
struct CameraFrame
{
    //! \brief World-to-camera matrix.
    Matrix44f view{ matrix::Identity };
    //! \brief Camera-to-clip matrix.
    Matrix44f projection{ matrix::Identity };
    //! \brief \c view * \c projection, the row-vector composition this
    //! library uses: a point \c p is transformed as \c p * view * projection.
    Matrix44f view_projection{ matrix::Identity };
    //! \brief Inverse of \c view, equal to the camera Entity's world matrix.
    Matrix44f inverse_view{ matrix::Identity };
    //! \brief Inverse of \c projection.
    Matrix44f inverse_projection{ matrix::Identity };
    //! \brief Where the camera sits, in world space.
    Vector3f position{ 0.0f, 0.0f, 0.0f };
    //! \brief The six planes of the visible volume, for culling.
    Frustum frustum{};

    // ------------------------------------------------------------------------
    //! \brief A world-space ray through a pixel, counting from the bottom
    //! left the way the graphics API counts.
    //!
    //! \param[in] p_x,p_y pixel coordinates, in the same space as
    //! \c Frame::mouse.
    //! \param[in] p_width,p_height size of the viewport this frame was
    //! extracted for. Zero on either axis yields a ray looking along the
    //! camera forward, so a forgotten size does not produce a NaN.
    // ------------------------------------------------------------------------
    [[nodiscard]] Ray screenRay(float p_x,
                                float p_y,
                                std::uint32_t p_width,
                                std::uint32_t p_height) const;
};

} // namespace render
