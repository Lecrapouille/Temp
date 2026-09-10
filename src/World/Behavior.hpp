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

namespace world
{
class World;
class Entity;
struct InputState;
} // namespace world

namespace world
{

using BehaviorCallback = void (*)(World& p_world,
                                  Entity p_entity,
                                  float p_dt,
                                  InputState const& p_input,
                                  void* p_user);

// ****************************************************************************
//! \brief Per-entity update hook. Not serializable: callbacks are runtime only.
// ****************************************************************************
struct Behavior
{
    BehaviorCallback callback = nullptr;
    void* user = nullptr;
};

} // namespace world
