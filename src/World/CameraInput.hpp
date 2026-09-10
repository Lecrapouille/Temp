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
//! \brief What a camera controller reads in one frame.
//!
//! Controllers never talk to a windowing library. The application fills this
//! from GLFW, SDL, a test harness or a recorded demo; the controller only
//! writes a LocalTransform. That is why the same OrbitController can be
//! driven by a unit test without opening a window.
// ****************************************************************************
struct CameraInput
{
    //! \brief Mouse motion this frame, in pixels, y up. Look is typically
    //! applied only when \c look is true.
    Vector2f look_delta{ 0.0f, 0.0f };
    //! \brief Scroll wheel this frame. Positive is "away from the user",
    //! which an orbit controller treats as zoom in.
    float zoom_delta = 0.0f;
    //! \brief The user is dragging to look (right mouse, captured cursor, …).
    bool look = false;
    //! \brief Translation keys. Names are the intended direction in camera
    //! space, not the physical key: the application maps WASD/QE itself.
    bool forward = false;
    bool back = false;
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    //! \brief Hold to multiply move speed (Left Shift in the examples).
    bool boost = false;
};

} // namespace world
