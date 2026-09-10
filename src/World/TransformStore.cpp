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

#include "World/TransformStore.hpp"

#include "Math/Transformation.hpp"

namespace world
{

namespace
{
const Matrix44f IDENTITY(matrix::Identity);
} // namespace

//------------------------------------------------------------------------------
Matrix44f LocalTransform::matrix() const
{
    // Translate, then rotate, then scale, the standard TRS order. That is
    // what Three.js and Unity compose parent-then-child with, and what lets
    // scale be inherited by children through the world-matrix multiplication.
    Matrix44f M = matrix::translate(Matrix44f(matrix::Identity), position);
    Quatf turning = rotation;
    M = matrix::rotate(M, turning.angle(), turning.axis());
    M = matrix::scale(M, scale);
    return M;
}

//------------------------------------------------------------------------------
void TransformStore::ensureCapacity(std::size_t p_index)
{
    if (p_index >= m_flags.size())
    {
        m_local.resize(p_index + 1u);
        m_world.resize(p_index + 1u, IDENTITY);
        m_flags.resize(p_index + 1u, 0u);
        m_generation.resize(p_index + 1u, 0u);
    }
}

//------------------------------------------------------------------------------
void TransformStore::allocate(Entity p_entity)
{
    assert(p_entity.valid() && "TransformStore::allocate on an empty entity");
    const std::size_t index = p_entity.index();
    ensureCapacity(index);

    // A slot reused by another generation must not silently keep the previous
    // TRS. Reset it before use.
    if ((m_generation[index] != p_entity.generation()) ||
        ((m_flags[index] & FLAG_PRESENT) == 0u))
    {
        m_local[index] = LocalTransform{};
        m_world[index] = IDENTITY;
    }

    m_flags[index] = static_cast<std::uint8_t>(FLAG_PRESENT | FLAG_DIRTY);
    m_generation[index] = p_entity.generation();
}

//------------------------------------------------------------------------------
void TransformStore::release(Entity p_entity)
{
    if (!p_entity.valid())
    {
        return;
    }
    const std::size_t index = p_entity.index();
    if (index >= m_flags.size())
    {
        return;
    }
    if (m_generation[index] != p_entity.generation())
    {
        return;
    }
    m_flags[index] = 0u;
    m_generation[index] = 0u;
    m_local[index] = LocalTransform{};
    m_world[index] = IDENTITY;
}

//------------------------------------------------------------------------------
bool TransformStore::has(Entity p_entity) const
{
    if (!p_entity.valid())
    {
        return false;
    }
    const std::size_t index = p_entity.index();
    if (index >= m_flags.size())
    {
        return false;
    }
    if (m_generation[index] != p_entity.generation())
    {
        return false;
    }
    return (m_flags[index] & FLAG_PRESENT) != 0u;
}

//------------------------------------------------------------------------------
LocalTransform const& TransformStore::local(Entity p_entity) const
{
    assert(has(p_entity) && "TransformStore::local on an entity without one");
    return m_local[p_entity.index()];
}

//------------------------------------------------------------------------------
LocalTransform& TransformStore::localMutable(Entity p_entity)
{
    assert(has(p_entity) &&
           "TransformStore::localMutable on an entity without one");
    return m_local[p_entity.index()];
}

//------------------------------------------------------------------------------
Matrix44f const& TransformStore::world(Entity p_entity) const
{
    assert(has(p_entity) && "TransformStore::world on an entity without one");
    return m_world[p_entity.index()];
}

//------------------------------------------------------------------------------
void TransformStore::setWorld(Entity p_entity, Matrix44f const& p_matrix)
{
    assert(has(p_entity) && "TransformStore::setWorld on an entity without one");
    m_world[p_entity.index()] = p_matrix;
}

//------------------------------------------------------------------------------
void TransformStore::markDirty(Entity p_entity)
{
    if (!has(p_entity))
    {
        return;
    }
    m_flags[p_entity.index()] |= FLAG_DIRTY;
}

//------------------------------------------------------------------------------
void TransformStore::markClean(Entity p_entity)
{
    if (!has(p_entity))
    {
        return;
    }
    m_flags[p_entity.index()] &=
        static_cast<std::uint8_t>(~FLAG_DIRTY);
}

//------------------------------------------------------------------------------
bool TransformStore::isDirty(Entity p_entity) const
{
    if (!has(p_entity))
    {
        return false;
    }
    return (m_flags[p_entity.index()] & FLAG_DIRTY) != 0u;
}

} // namespace world
