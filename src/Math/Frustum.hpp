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

#include "Math/AABB.hpp"
#include "Math/Matrix.hpp"
#include "Math/Vector.hpp"

#include <array>
#include <cmath>

// ****************************************************************************
//! \file
//! \brief The six planes of a view, for asking whether a box is visible.
//!
//! Built from a view-projection matrix, so any camera that can write that
//! matrix can be culled against. The renderer does not need to know how the
//! camera was aimed.
// ****************************************************************************

// ****************************************************************************
//! \brief One plane: ax + by + cz + d = 0, with (a,b,c) of length one.
//!
//! The half-space in front of the camera is ax + by + cz + d >= 0.
// ****************************************************************************
struct Plane
{
    Vector3f normal{ 0.0f, 0.0f, 1.0f };
    float offset = 0.0f;

    [[nodiscard]] float signedDistance(Vector3f const& p_point) const
    {
        return (normal.x * p_point.x) + (normal.y * p_point.y) +
               (normal.z * p_point.z) + offset;
    }
};

// ****************************************************************************
//! \brief The volume a camera can see, as six planes.
// ****************************************************************************
struct Frustum
{
    std::array<Plane, 6u> planes{};

    // ------------------------------------------------------------------------
    //! \brief Read the six planes out of a view-projection matrix.
    //!
    //! This library uses the row-vector convention: a point p is a 1x4 row
    //! and applying view then projection to p is p * view * projection. The
    //! caller therefore passes \c view * \c projection, not the other way
    //! round. The extraction uses the standard Gribb/Hartmann form
    //! (row 3 +/- row i of the combined matrix).
    // ------------------------------------------------------------------------
    [[nodiscard]] static Frustum fromViewProjection(Matrix44f const& p_vp)
    {
        auto make = [](Vector4f const& p_row) {
            Plane plane;
            const float length = std::sqrt((p_row.x * p_row.x) +
                                           (p_row.y * p_row.y) +
                                           (p_row.z * p_row.z));
            const float inv =
                (length < 1.0e-8f) ? 1.0f : (1.0f / length);
            plane.normal = Vector3f(p_row.x * inv, p_row.y * inv, p_row.z * inv);
            plane.offset = p_row.w * inv;
            return plane;
        };

        // Gribb/Hartmann wants the rows of the matrix the shader multiplies
        // with. This library stores those as columns: CPU row i is shader
        // column i, so shader row r is (M[0][r], M[1][r], M[2][r], M[3][r]).
        auto shaderRow = [&p_vp](std::size_t p_row) {
            return Vector4f(p_vp[0][p_row],
                            p_vp[1][p_row],
                            p_vp[2][p_row],
                            p_vp[3][p_row]);
        };

        Frustum frustum;
        frustum.planes[0] = make(shaderRow(3) + shaderRow(0)); // left
        frustum.planes[1] = make(shaderRow(3) - shaderRow(0)); // right
        frustum.planes[2] = make(shaderRow(3) + shaderRow(1)); // bottom
        frustum.planes[3] = make(shaderRow(3) - shaderRow(1)); // top
        frustum.planes[4] = make(shaderRow(3) + shaderRow(2)); // near
        frustum.planes[5] = make(shaderRow(3) - shaderRow(2)); // far
        return frustum;
    }

    // ------------------------------------------------------------------------
    //! \brief Is any part of this box in front of every plane?
    //!
    //! An empty box is not visible: there is nothing to draw.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool contains(AABB const& p_box) const
    {
        if (p_box.empty())
        {
            return false;
        }

        for (Plane const& plane : planes)
        {
            // The corner furthest along the plane normal. If that one is
            // behind the plane, the whole box is.
            const Vector3f corner(
                (plane.normal.x >= 0.0f) ? p_box.max.x : p_box.min.x,
                (plane.normal.y >= 0.0f) ? p_box.max.y : p_box.min.y,
                (plane.normal.z >= 0.0f) ? p_box.max.z : p_box.min.z);
            if (plane.signedDistance(corner) < 0.0f)
            {
                return false;
            }
        }
        return true;
    }
};
