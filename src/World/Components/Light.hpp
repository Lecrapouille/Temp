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

#include "Math/Vector.hpp"

namespace world
{

// ****************************************************************************
//! \brief A light that does not fall off with distance.
//!
//! Where it points is the Entity's forward axis (the local \c -Z direction of
//! the world matrix), so a directional light parented to a rig turns with the
//! rig. Rotate the entity to aim the light.
// ****************************************************************************
struct DirectionalLight
{
    Vector3f color{ 1.0f, 1.0f, 1.0f };
    float intensity = 1.0f;
};

// ****************************************************************************
//! \brief A light at a point, falling off with distance.
//!
//! The point in space is the Entity's world position. The renderer reads the
//! entity's world matrix at extraction time and passes the position to the
//! shader; the component itself holds only what does not come from the
//! transform.
// ****************************************************************************
struct PointLight
{
    Vector3f color{ 1.0f, 1.0f, 1.0f };
    float intensity = 1.0f;
    //! \brief Distance at which the intensity has fallen to about 1%. Meant
    //! for a physically inspired 1/(1 + kd^2) shader.
    float range = 100.0f;
};

} // namespace world
