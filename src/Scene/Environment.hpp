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

namespace scene
{

// ****************************************************************************
//! \brief The world's ambient information: what a Scene tells the renderer
//! about the space around the entities.
//!
//! Kept small on purpose. IBL, skybox and HDR belong here when Stage 9 is
//! reached; the fields below are what the built-in lit material reads today.
// ****************************************************************************
struct Environment
{
    //! \brief A dim colour multiplied by the material colour to fake the light
    //! that comes from every direction.
    Vector3f ambient{ 0.15f, 0.15f, 0.18f };

    //! \brief Direction the built-in lit shader receives when no directional
    //! light is present in the World. Points toward the light (i.e. the
    //! opposite of what the photons travel).
    Vector3f default_light_direction{ 0.4f, 0.8f, 0.6f };
};

} // namespace scene
