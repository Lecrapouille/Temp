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
#include "GPU/Pipeline.hpp"
#include "GPU/Shader.hpp"
#include "Math/Vector.hpp"

#include <string>

namespace assets
{

// ****************************************************************************
//! \brief Which built-in shader family a Material was built from.
// ****************************************************************************
enum class ShaderFamily
{
    //! \brief Single directional light, flat \c color uniform.
    Lit,
    //! \brief glTF-style base colour factor + optional albedo map.
    PbrMinimal,
    //! \brief Legacy DepthMaterial: linearised eye-space depth as greyscale.
    Depth,
    //! \brief Legacy NormalsMaterial: world-space normal as RGB.
    Normals,
};

// ****************************************************************************
//! \brief A shader family with its render state. The heavy part shared by many
//! objects that draw the same way.
//!
//! A Material owns its \c gpu::Program and its \c gpu::Pipeline. Many
//! MaterialInstances can point at the same Material, and a Material is what a
//! renderer would key its "which pipeline is bound right now" cache on.
// ****************************************************************************
struct Material
{
    //! \brief A short label, used in logs.
    std::string name;
    //! \brief Which ShaderLib sources this material was built from.
    ShaderFamily family = ShaderFamily::Lit;
    //! \brief The linked GLSL program.
    gpu::Program program;
    //! \brief The pipeline: program + vertex layout + render state, checked
    //! against each other once at load time.
    gpu::Pipeline pipeline;
};

// ****************************************************************************
//! \brief Per-object parameters of a Material.
//!
//! A MaterialInstance holds only the values that vary between two objects
//! drawn with the same shader: a colour, texture bindings, tuning uniforms.
//! It does not own the pipeline, does not compile a shader and does not change
//! render state.
//!
//! Two objects sharing the same MaterialInstance are drawn identically. The
//! renderer batches them together.
// ****************************************************************************
struct MaterialInstance
{
    //! \brief Which shader/pipeline family this instance draws with.
    MaterialId material;
    //! \brief The base colour uploaded to the shader's \c color uniform, when
    //! the shader declares one.
    Vector3f color{ 1.0f, 1.0f, 1.0f };
    //! \brief PBR base colour factor (\c baseColorFactor in glTF).
    Vector3f base_color_factor{ 1.0f, 1.0f, 1.0f };
    //! \brief Optional albedo map. Empty means the factor alone is used.
    TextureAssetId base_color_texture{};
    //! \brief DepthMaterial near/far smoothstep range, in eye-space metres.
    float depth_near = 1.0f;
    float depth_far = 100.0f;
    //! \brief Alpha for depth and normals debug materials.
    float opacity = 1.0f;
};

} // namespace assets
