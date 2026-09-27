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

#include "Compages/Math/Vector.hpp"

namespace world
{

// ****************************************************************************
//! \brief How the physics step may move this entity's transform.
// ****************************************************************************
enum class BodyType
{
    //! \brief Never moved by physics.
    Static,
    //! \brief Moved by forces and collisions.
    Dynamic,
    //! \brief Transform is written externally; physics reads but does not
    //! integrate.
    Kinematic,
};

// ****************************************************************************
//! \brief Linear motion state for a physics body.
// ****************************************************************************
struct RigidBody
{
    BodyType type = BodyType::Dynamic;
    Vector3f velocity{ 0.0f, 0.0f, 0.0f };
    float mass = 1.0f;
    Vector3f angular_velocity{ 0.0f, 0.0f, 0.0f };
    float linear_damping = 0.05f;
    float angular_damping = 0.05f;
    float friction = 0.3f;
    float restitution = 0.0f;
    bool gravity_enabled = true;
};

} // namespace world
