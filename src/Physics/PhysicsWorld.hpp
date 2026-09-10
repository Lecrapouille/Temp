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
class EventQueue;
class World;
}

namespace physics
{

// ****************************************************************************
//! \brief Minimal AABB physics for MVP demos.
//!
//! Integrates dynamic bodies, separates overlapping AABBs along the smallest
//! axis (A moves away from B) and writes positions back into LocalTransform.
//! Physics owns velocity; the caller owns rotation and scale.
// ****************************************************************************
class PhysicsWorld
{
public:

    Vector3f gravity{ 0.0f, -9.81f, 0.0f };

    void step(world::World& p_world, float p_dt, world::EventQueue* p_events);
};

} // namespace physics
