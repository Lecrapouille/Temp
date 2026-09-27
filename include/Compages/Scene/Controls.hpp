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

#include "Compages/Scene/CameraInput.hpp"
#include "Compages/Scene/FlyController.hpp"
#include "Compages/Scene/OrbitController.hpp"
#include "Compages/Scene/World.hpp"

// ****************************************************************************
//! \file
//! \brief Camera controls, as behaviors added to the camera entity.
//!
//! \code
//! m_scene.camera().position(0, 2, 6).add<scene::Orbit>();   // look at 0,0,0
//! \endcode
//!
//! They start from wherever the entity already is, so placing the camera
//! first and adding the control after keeps the placement.
// ****************************************************************************

namespace scene
{

// ****************************************************************************
//! \brief Turn around a target with the right mouse button, zoom with the
//! wheel.
// ****************************************************************************
struct Orbit : Behavior
{
    //! \brief The tuning: the target, the sensitivities, the limits.
    OrbitController controller;

    Orbit() = default;
    explicit Orbit(Vector3f p_target) { controller.target = p_target; }

    //! \brief Seed distance, zoom limits and yaw/pitch from the camera pose.
    void start() override;

    //! \brief Apply orbit input; optional \c spin when the user is not looking.
    //! \param[in] p_dt Seconds since the previous frame.
    void update(float p_dt) override;

    //! \brief Radians a second the camera turns by itself while the user
    //! does not turn it, like a turntable. Zero keeps it still.
    float spin = 0.0f;
};

// ****************************************************************************
//! \brief Fly freely: the right mouse button looks around, W A S D move, E
//! and Q go up and down, Shift runs.
// ****************************************************************************
struct Fly : Behavior
{
    //! \brief The tuning: the speeds, the sensitivity, the limits.
    FlyController controller;

    //! \brief Seed yaw/pitch from the camera's current forward (-Z).
    void start() override;

    //! \brief Apply fly look and movement from keyboard and mouse input.
    //! \param[in] p_dt Seconds since the previous frame.
    void update(float p_dt) override;
};

} // namespace scene
