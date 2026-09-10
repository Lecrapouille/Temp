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

// ****************************************************************************
//! \file
//! \brief The single header to include to use the GPU layer.
//!
//! \code
//! #include "GPU/GPU.hpp"
//!
//! // The window is yours to create; the library only wants the loader.
//! if (auto ready = gpu::init(glfwGetProcAddress); !ready)
//! {
//!     std::cerr << ready.error() << std::endl;
//!     return EXIT_FAILURE;
//! }
//! ...
//! gpu::shutdown();
//! \endcode
//!
//! Nothing here exposes the graphics API in use. That is deliberate and
//! enforced: the only file allowed to include glad is the private header of the
//! backend. See doc/Design.md.
// ****************************************************************************

#include "GPU/Buffer.hpp"
#include "GPU/Compute.hpp"
#include "GPU/Core/DirtyRange.hpp"
#include "GPU/Core/Enums.hpp"
#include "GPU/Core/Handle.hpp"
#include "GPU/Core/Layout.hpp"
#include "GPU/Core/PixelFormat.hpp"
#include "GPU/Core/Reflection.hpp"
#include "GPU/Core/RenderState.hpp"
#include "GPU/Core/Result.hpp"
#include "GPU/Core/Std140.hpp"
#include "GPU/Device.hpp"
#include "GPU/Draw.hpp"
#include "GPU/Framebuffer.hpp"
#include "GPU/Pipeline.hpp"
#include "GPU/RenderPass.hpp"
#include "GPU/Shader.hpp"
#include "GPU/Statistics.hpp"
#include "GPU/Texture.hpp"
#include "GPU/UniformBlock.hpp"
#include "GPU/VertexArray.hpp"
