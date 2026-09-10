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

#include <vector>

namespace world
{

// ****************************************************************************
//! \brief Joint Entities of a skinned MeshRenderer, in skin order.
//!
//! Lives on the same Entity as the MeshRenderer. The MeshAsset holds the
//! inverse-bind matrices and the rest-pose vertices; this component only
//! names which World nodes are the current pose.
// ****************************************************************************
struct SkinInstance
{
    std::vector<Entity> joints;
};

} // namespace world
