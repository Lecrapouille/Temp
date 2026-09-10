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

#include "GPU/Statistics.hpp"

// ****************************************************************************
//! \file
//! \brief How the counters of the frame are written to.
//!
//! Internal, so that the numbers a caller reads can only be changed by the calls
//! that really did the work.
// ****************************************************************************

namespace gpu::detail
{

// ----------------------------------------------------------------------------
//! \brief Record one draw call.
//!
//! \param[in] p_vertices how many vertices or indices it asked for.
//! \param[in] p_instances how many objects it drew.
// ----------------------------------------------------------------------------
void countDraw(std::size_t p_vertices, std::size_t p_instances);

// ----------------------------------------------------------------------------
//! \brief Record one pass being opened.
// ----------------------------------------------------------------------------
void countPass();

// ----------------------------------------------------------------------------
//! \brief Record one compute dispatch.
// ----------------------------------------------------------------------------
void countDispatch();

} // namespace gpu::detail
