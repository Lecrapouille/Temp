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
#include "GPU/Buffer.hpp"
#include "GPU/Core/Enums.hpp"
#include "Math/AABB.hpp"
#include "Math/Vector.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace assets
{

// ****************************************************************************
//! \brief What one corner of a mesh looks like for the built-in lit shader.
//!
//! This is the vertex the AssetManager fills for the primitives it can build
//! by itself. A GLB loader will produce vertices of its own struct that share
//! this same layout, so the same pipeline can draw both.
// ****************************************************************************
struct MeshVertex
{
    Vector3f position;
    Vector3f normal;
    //! \brief Texture coordinates. Unused by the lit shader; required by PBR
    //! and by meshes imported from glTF.
    Vector2f uv{ 0.0f, 0.0f };
};

// ****************************************************************************
//! \brief The GPU-side realization of a mesh, plus its local bounds.
//!
//! Owned by the AssetManager. World components refer to it by MeshAssetId, not
//! by pointer or handle: the AssetManager is what keeps the buffers alive.
// ****************************************************************************
struct MeshAsset
{
    //! \brief Vertex buffer holding \c MeshVertex records.
    gpu::Buffer<MeshVertex> vertices;
    //! \brief Index buffer, 16-bit indices.
    gpu::Buffer<std::uint16_t> indices;
    //! \brief Number of indices, cached to avoid a per-draw query.
    std::size_t index_count = 0u;
    //! \brief Kind of indices in the buffer. UInt16 for anything below 65536
    //! vertices, which is every primitive built here.
    gpu::IndexType index_type = gpu::IndexType::UInt16;
    //! \brief Bounding box of the mesh in its own local space. What the
    //! renderer transforms per instance before culling.
    AABB local_bounds;
    //! \brief Inverse-bind skeleton, empty when the mesh is rigid.
    SkinAssetId skin{};
    //! \brief Rest-pose vertices used to rebuild \c vertices each frame when
    //! the mesh is skinned. Empty for a rigid mesh.
    std::vector<MeshVertex> rest_pose;
    //! \brief Four joint indices per rest-pose vertex, matching \c rest_pose.
    std::vector<std::uint16_t> joint_indices;
    //! \brief Four weights per rest-pose vertex, matching \c rest_pose.
    std::vector<Vector4f> joint_weights;
};

} // namespace assets
