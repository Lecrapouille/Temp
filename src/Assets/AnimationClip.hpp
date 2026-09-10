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

#include "World/Entity.hpp"

#include <string>
#include <vector>

namespace assets
{

enum class AnimationPath
{
    Translation,
    Rotation,
    Scale,
};

enum class AnimationInterpolation
{
    Step,
    Linear,
};

// ****************************************************************************
//! \brief One sampled curve that writes a local TRS channel of an Entity.
//!
//! Times are in seconds. Values are packed: three floats per key for
//! translation and scale, four (glTF xyzw) for rotation.
// ****************************************************************************
struct AnimationChannel
{
    world::Entity target{};
    AnimationPath path = AnimationPath::Translation;
    AnimationInterpolation interpolation = AnimationInterpolation::Linear;
    std::vector<float> times;
    std::vector<float> values;
};

// ****************************************************************************
//! \brief A named clip imported from a glTF animation.
// ****************************************************************************
struct AnimationClip
{
    std::string name;
    float duration = 0.0f;
    std::vector<AnimationChannel> channels;
};

} // namespace assets
