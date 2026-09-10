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

#include "Math/Matrix.hpp"
#include "Math/Quaternion.hpp"
#include "Math/Vector.hpp"
#include "World/Entity.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace world
{

// ****************************************************************************
//! \brief Place, attitude and size of an Entity, relative to its parent.
//!
//! Position, rotation and scale are the primary data. The world matrix is a
//! derived cache computed by \c TransformSystem out of these three and of the
//! parent's world matrix.
//!
//! Scale is included in the local matrix and inherited by children through the
//! standard TRS composition
//! \code
//! WorldMatrix(child) = WorldMatrix(parent) * LocalMatrix(child)
//! \endcode
//! which is what Three.js and Unity do. A scene needing an unscaled joint
//! between two scaled meshes uses an intermediate entity with scale (1, 1, 1),
//! not a hidden exception to the composition rule.
//!
//! No write-authority flag is stored per transform. Who is allowed to write a
//! transform during which phase of the frame is a system/scheduler concern; a
//! flag next to the data invites silent last-writer-wins bugs and is not the
//! way this codebase reads.
// ****************************************************************************
struct LocalTransform
{
    Vector3f position{ 0.0f, 0.0f, 0.0f };
    Quatf rotation{};
    Vector3f scale{ 1.0f, 1.0f, 1.0f };

    // ------------------------------------------------------------------------
    //! \brief Translate, then rotate, then scale, the standard TRS matrix.
    //!
    //! This is what \c TransformSystem multiplies with the parent's world
    //! matrix to build the world matrix of a child, so scale is inherited by
    //! default.
    // ------------------------------------------------------------------------
    [[nodiscard]] Matrix44f matrix() const;
};

// ****************************************************************************
//! \brief SoA storage for local TRS and derived world matrices, keyed by
//! Entity index.
//!
//! Every array is indexed by \c Entity::index() and grows as new spatial
//! entities appear. An entity may exist without a transform; only those that
//! are added through \c allocate() have one. Whether an entity is spatial is
//! not decided by this store: the World layer allocates a transform for every
//! spatial entity, and neither for the ones that are only data.
//!
//! Dirty tracking is per entity. Marking descendants dirty needs the
//! \c SpatialGraph and is the World's job; the store only records "someone
//! wrote to this entity's TRS since the last update".
// ****************************************************************************
class TransformStore
{
public:

    TransformStore() = default;
    TransformStore(TransformStore const&) = delete;
    TransformStore& operator=(TransformStore const&) = delete;
    TransformStore(TransformStore&&) = default;
    TransformStore& operator=(TransformStore&&) = default;

    // ------------------------------------------------------------------------
    //! \brief Allocate a transform slot for an Entity if none exists.
    //!
    //! Idempotent. Fills the local pose with identity and the world matrix
    //! with identity as well, and marks the entity dirty so the first update
    //! computes a real world matrix even on a hierarchy of one node.
    // ------------------------------------------------------------------------
    void allocate(Entity p_entity);

    // ------------------------------------------------------------------------
    //! \brief Release the slot of an Entity.
    //!
    //! Called by the World when the entity is destroyed. Does not shrink the
    //! arrays: the parallel indexing on \c Entity::index() must stay stable
    //! for reused slots.
    // ------------------------------------------------------------------------
    void release(Entity p_entity);

    // ------------------------------------------------------------------------
    //! \brief Does this entity have a transform?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool has(Entity p_entity) const;

    // ------------------------------------------------------------------------
    //! \brief The local pose. Const access; does not mark anything dirty.
    // ------------------------------------------------------------------------
    [[nodiscard]] LocalTransform const& local(Entity p_entity) const;

    // ------------------------------------------------------------------------
    //! \brief Mutable local pose. The caller is responsible for calling
    //! \c markDirty() afterward, which the World does on the caller's behalf
    //! when the local is obtained through \c World::transform().
    // ------------------------------------------------------------------------
    [[nodiscard]] LocalTransform& localMutable(Entity p_entity);

    // ------------------------------------------------------------------------
    //! \brief The derived world matrix at the last \c TransformSystem update.
    // ------------------------------------------------------------------------
    [[nodiscard]] Matrix44f const& world(Entity p_entity) const;

    // ------------------------------------------------------------------------
    //! \brief Overwrite the derived world matrix. Called only by
    //! \c TransformSystem while it walks the hierarchy.
    // ------------------------------------------------------------------------
    void setWorld(Entity p_entity, Matrix44f const& p_matrix);

    // ------------------------------------------------------------------------
    //! \brief Say that the local pose of this entity has changed since the
    //! last update.
    // ------------------------------------------------------------------------
    void markDirty(Entity p_entity);

    // ------------------------------------------------------------------------
    //! \brief Clear the dirty flag of an entity.
    // ------------------------------------------------------------------------
    void markClean(Entity p_entity);

    // ------------------------------------------------------------------------
    //! \brief Is this entity marked dirty?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool isDirty(Entity p_entity) const;

    // ------------------------------------------------------------------------
    //! \brief How many slots exist, alive or free. The parallel arrays are all
    //! of this size.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t capacity() const
    {
        return m_flags.size();
    }

private:

    static constexpr std::uint8_t FLAG_PRESENT = 0x01u;
    static constexpr std::uint8_t FLAG_DIRTY = 0x02u;

    void ensureCapacity(std::size_t p_index);

    std::vector<LocalTransform> m_local;
    std::vector<Matrix44f> m_world;
    //! \brief Per-slot flags: bit 0 says whether the slot holds a transform,
    //! bit 1 says whether it is dirty.
    std::vector<std::uint8_t> m_flags;
    //! \brief Generation of the Entity that last used the slot, so releasing
    //! or reusing a slot from another generation is caught rather than
    //! silently reused.
    std::vector<std::uint16_t> m_generation;
};

} // namespace world
