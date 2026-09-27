//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#pragma once

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Vector.hpp"
#include "Compages/Scene/EntityId.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace scene
{

// ****************************************************************************
//! \brief Translate, then rotate, then scale — the standard TRS matrix.
//!
//! Shared by the value type, the SoA view, and \c TransformSystem so a hot
//! update never has to pack position/rotation/scale back into a struct.
// ****************************************************************************
[[nodiscard]] Matrix44f composeLocalMatrix(Vector3f const& p_position,
                                           Quatf const& p_rotation,
                                           Vector3f const& p_scale);

// ****************************************************************************
//! \brief Place, attitude and size of an EntityId, relative to its parent.
//!
//! This is the *value* type: a snapshot you copy, assign, serialise or put
//! inside a prefab. Storage in \c TransformStore is SoA (three parallel
//! arrays). \c World::transform() returns a \c LocalTransformView that
//! writes those arrays directly.
// ****************************************************************************
struct LocalTransform
{
    Vector3f position{ 0.0f, 0.0f, 0.0f };
    Quatf rotation{};
    Vector3f scale{ 1.0f, 1.0f, 1.0f };

    [[nodiscard]] Matrix44f matrix() const
    {
        return composeLocalMatrix(position, rotation, scale);
    }
};

// ****************************************************************************
//! \brief Mutable window onto one entity's SoA slot.
//!
//! \c position, \c rotation and \c scale are references into the store, so
//! \code
//! world.transform(e).position = { 1, 0, 0 };
//! world.transform(e) = LocalTransform{};
//! \endcode
//! keep working without packing the three fields back into an AoS array.
// ****************************************************************************
class LocalTransformView
{
public:

    Vector3f& position;
    Quatf& rotation;
    Vector3f& scale;

    LocalTransformView(Vector3f& p_position,
                       Quatf& p_rotation,
                       Vector3f& p_scale)
        : position(p_position), rotation(p_rotation), scale(p_scale)
    {
    }

    LocalTransformView(LocalTransformView const&) = default;

    LocalTransformView& operator=(LocalTransformView const& p_other)
    {
        position = p_other.position;
        rotation = p_other.rotation;
        scale = p_other.scale;
        return *this;
    }

    LocalTransformView& operator=(LocalTransform const& p_value)
    {
        position = p_value.position;
        rotation = p_value.rotation;
        scale = p_value.scale;
        return *this;
    }

    [[nodiscard]] operator LocalTransform() const
    {
        return LocalTransform{ position, rotation, scale };
    }

    [[nodiscard]] Matrix44f matrix() const
    {
        return composeLocalMatrix(position, rotation, scale);
    }

    // ------------------------------------------------------------------------
    //! \brief Turn by an angle around an axis of the entity itself, as
    //! three.js' Object3D.rotateOnAxis() does.
    //!
    //! \code
    //! transform().rotate(p_dt, { 0.0f, 1.0f, 0.0f });   // one radian a second
    //! \endcode
    // ------------------------------------------------------------------------
    LocalTransformView& rotate(float p_radians, Vector3f const& p_axis)
    {
        rotation = rotation * Quatf::fromAngleAxis(units::angle::radian_t(p_radians),
                                                   vector::normalize(p_axis));
        rotation.normalize();
        return *this;
    }

    //! \brief Turn around the entity's own x axis.
    LocalTransformView& rotateX(float p_radians)
    {
        return rotate(p_radians, Vector3f(1.0f, 0.0f, 0.0f));
    }

    //! \brief Turn around the entity's own y axis: the one a spinning top
    //! turns around.
    LocalTransformView& rotateY(float p_radians)
    {
        return rotate(p_radians, Vector3f(0.0f, 1.0f, 0.0f));
    }

    //! \brief Turn around the entity's own z axis.
    LocalTransformView& rotateZ(float p_radians)
    {
        return rotate(p_radians, Vector3f(0.0f, 0.0f, 1.0f));
    }

    //! \brief Move by an offset, expressed in the parent's axes.
    LocalTransformView& translate(Vector3f const& p_offset)
    {
        position += p_offset;
        return *this;
    }
};

// ****************************************************************************
//! \brief SoA storage for local TRS and derived world matrices, keyed by
//! EntityId index.
//!
//! Position, rotation and scale live in three parallel arrays. Walking a
//! system that only needs positions (physics integration, a translation
//! channel) therefore reads a tight float3 stream instead of skipping a
//! quaternion and a scale at every stride.
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

    void allocate(EntityId p_entity);
    void release(EntityId p_entity);

    [[nodiscard]] bool has(EntityId p_entity) const;

    // ------------------------------------------------------------------------
    //! \brief A copy of the local pose. Does not mark anything dirty.
    // ------------------------------------------------------------------------
    [[nodiscard]] LocalTransform local(EntityId p_entity) const;

    // ------------------------------------------------------------------------
    //! \brief Mutable window onto the SoA slot. The caller (or \c World) must
    //! \c markDirty() after writing.
    // ------------------------------------------------------------------------
    [[nodiscard]] LocalTransformView localMutable(EntityId p_entity);

    [[nodiscard]] Vector3f const& position(EntityId p_entity) const;
    [[nodiscard]] Quatf const& rotation(EntityId p_entity) const;
    [[nodiscard]] Vector3f const& scale(EntityId p_entity) const;

    [[nodiscard]] std::span<Vector3f const> positions() const
    {
        return m_position;
    }
    [[nodiscard]] std::span<Quatf const> rotations() const
    {
        return m_rotation;
    }
    [[nodiscard]] std::span<Vector3f const> scales() const
    {
        return m_scale;
    }
    [[nodiscard]] std::span<Matrix44f const> worlds() const
    {
        return m_world;
    }

    [[nodiscard]] Matrix44f localMatrix(EntityId p_entity) const;

    [[nodiscard]] Matrix44f const& world(EntityId p_entity) const;
    void setWorld(EntityId p_entity, Matrix44f const& p_matrix);

    void markDirty(EntityId p_entity);
    void markClean(EntityId p_entity);
    [[nodiscard]] bool isDirty(EntityId p_entity) const;

    [[nodiscard]] std::size_t capacity() const
    {
        return m_flags.size();
    }

private:

    static constexpr std::uint8_t FLAG_PRESENT = 0x01u;
    static constexpr std::uint8_t FLAG_DIRTY = 0x02u;

    void ensureCapacity(std::size_t p_index);
    [[nodiscard]] bool slotMatches(EntityId p_entity) const;

    std::vector<Vector3f> m_position;
    std::vector<Quatf> m_rotation;
    std::vector<Vector3f> m_scale;
    std::vector<Matrix44f> m_world;
    std::vector<std::uint8_t> m_flags;
    std::vector<std::uint16_t> m_generation;
};

} // namespace scene
