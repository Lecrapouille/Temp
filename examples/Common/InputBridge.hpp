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

#include "Common/Example.hpp"
#include "World/CameraInput.hpp"
#include "World/InputState.hpp"

namespace examples
{

inline void fillInputState(world::InputState& p_out, Frame const& p_frame)
{
    p_out.mouse = p_frame.mouse;
    p_out.mouse_delta = p_frame.mouse_delta;
    p_out.scroll = p_frame.scroll;
    p_out.mouse_left = p_frame.mouse_left;
    p_out.mouse_right = p_frame.mouse_right;
    p_out.mouse_left_pressed = p_frame.mouse_left_pressed;
    p_out.key_w = p_frame.key_w;
    p_out.key_a = p_frame.key_a;
    p_out.key_s = p_frame.key_s;
    p_out.key_d = p_frame.key_d;
    p_out.key_q = p_frame.key_q;
    p_out.key_e = p_frame.key_e;
    p_out.key_shift = p_frame.key_shift;
}

inline world::CameraInput toCameraInput(world::InputState const& p_input)
{
    world::CameraInput camera;
    camera.look_delta = p_input.mouse_delta;
    camera.zoom_delta = p_input.scroll;
    camera.look = p_input.mouse_right;
    camera.forward = p_input.key_w;
    camera.back = p_input.key_s;
    camera.left = p_input.key_a;
    camera.right = p_input.key_d;
    camera.up = p_input.key_e;
    camera.down = p_input.key_q;
    camera.boost = p_input.key_shift;
    return camera;
}

} // namespace examples
