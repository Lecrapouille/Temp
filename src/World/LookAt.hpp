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

#include "Math/Quaternion.hpp"
#include "Math/Transformation.hpp"
#include "World/TransformStore.hpp"

namespace world
{

// ****************************************************************************
//! \brief Orient an entity so its local \c -Z axis points at a world target.
//!
//! Matches the behaviour of \c Object3D.lookAt() in three.js: the position
//! stays put, only the rotation changes.
// ****************************************************************************
inline void lookAt(LocalTransform& p_transform,
                   Vector3f const& p_target,
                   Vector3f const& p_up = Vector3f(0.0f, 1.0f, 0.0f))
{
    const Vector3f delta = p_target - p_transform.position;
    const float length = vector::norm(delta);
    if (length < 1.0e-6f)
    {
        return;
    }

    const Matrix44f view =
        matrix::lookAt(p_transform.position, p_target, p_up);
    const Matrix44f object = matrix::inverse(view);
    p_transform.rotation = Quatf::fromMatrix(object);
}

} // namespace world
