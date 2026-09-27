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

#include "Compages/Core/Units.hpp"
#include "Compages/Scene/TransformStore.hpp"

#include <algorithm>
#include <limits>

// ****************************************************************************
//! \file
//! \brief The joints of a kinematic chain: what moves a link relative to its
//! parent link. A joint is a component of the child entity; the hierarchy
//! itself stays in the SpatialGraph.
//!
//! \code
//! scene::Entity base = world.entity("Base");
//! scene::Entity arm = base.child("Arm").position(0, 0, 0.5f)
//!                         .revolute({ 0, 0, 1 }, -90.0_deg, 90.0_deg);
//! arm.angle(30.0_deg);
//! \endcode
//!
//! The KinematicSystem turns each joint into the LocalTransform of its entity
//! before the TransformSystem computes the world matrices:
//! \code
//! Local = Origin * Rotation(axis, q)        // RevoluteJoint
//! Local = Origin * Translation(axis * q)    // PrismaticJoint
//! \endcode
//! A fixed joint needs no component: it is the plain LocalTransform.
// ****************************************************************************

namespace scene
{

// ****************************************************************************
//! \brief A value and the interval it must stay in. Unbounded by default.
//! \tparam U a unit type of nholthaus/units, as units::angle::radian_t.
// ****************************************************************************
template <class U>
struct Bounded
{
    U value{ 0.0 };
    U min{ -std::numeric_limits<double>::infinity() };
    U max{ std::numeric_limits<double>::infinity() };

    //! \brief The value brought back inside [min, max].
    [[nodiscard]] U clamped() const
    {
        return std::clamp(value, min, max);
    }
};

// ****************************************************************************
//! \brief Position, velocity and acceleration of a joint: the (q, v, a) of
//! Pinocchio. Only the position moves the link; the two others are bounds and
//! state for whoever plans or controls the motion.
// ****************************************************************************
template <class Pos, class Vel, class Acc>
struct JointState
{
    Bounded<Pos> position;
    Bounded<Vel> velocity;
    Bounded<Acc> acceleration;
};

using RevoluteState =
    JointState<units::angle::radian_t,
               units::angular_velocity::radians_per_second_t,
               units::angular_acceleration::radians_per_second_squared_t>;

using PrismaticState =
    JointState<units::length::meter_t,
               units::velocity::meters_per_second_t,
               units::acceleration::meters_per_second_squared_t>;

// ****************************************************************************
//! \brief Turns its link around an axis of the joint frame. A URDF
//! "continuous" joint is a revolute joint without bounds.
// ****************************************************************************
struct RevoluteJoint
{
    //! \brief Place of the joint frame relative to the parent link, when the
    //! angle is zero.
    LocalTransform origin{};
    //! \brief Unit axis of rotation, in the joint frame.
    Vector3f axis{ 0.0f, 0.0f, 1.0f };
    RevoluteState state{};
};

// ****************************************************************************
//! \brief Slides its link along an axis of the joint frame.
// ****************************************************************************
struct PrismaticJoint
{
    //! \brief Place of the joint frame relative to the parent link, when the
    //! offset is zero.
    LocalTransform origin{};
    //! \brief Unit axis of translation, in the joint frame.
    Vector3f axis{ 0.0f, 0.0f, 1.0f };
    PrismaticState state{};
};

//! \brief The local place of the link, for the clamped angle of the joint.
[[nodiscard]] LocalTransform jointTransform(RevoluteJoint const& p_joint);

//! \brief The local place of the link, for the clamped offset of the joint.
[[nodiscard]] LocalTransform jointTransform(PrismaticJoint const& p_joint);

} // namespace scene
