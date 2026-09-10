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

#include "Assets/AssetIds.hpp"

namespace world
{

// ****************************************************************************
//! \brief Plays one AnimationClip onto the Entities the clip's channels name.
//!
//! \c AnimationSystem advances \c time and writes local TRS. It does not own
//! the clip: the AssetManager does.
// ****************************************************************************
struct Animator
{
    assets::AnimationClipId clip{};
    float time = 0.0f;
    float speed = 1.0f;
    bool loop = true;
    bool playing = true;
};

} // namespace world
