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
//! \brief Platform-agnostic input snapshot for gameplay systems.
//!
//! Examples fill this from their window layer; Behaviors read it without
//! knowing about GLFW.
// ****************************************************************************
struct InputState
{
    Vector2f mouse{ 0.0f, 0.0f };
    Vector2f mouse_delta{ 0.0f, 0.0f };
    float scroll = 0.0f;
    bool mouse_left = false;
    bool mouse_right = false;
    bool mouse_left_pressed = false;
    bool key_w = false;
    bool key_a = false;
    bool key_s = false;
    bool key_d = false;
    bool key_q = false;
    bool key_e = false;
    bool key_shift = false;
    bool key_space = false;
};

} // namespace world
