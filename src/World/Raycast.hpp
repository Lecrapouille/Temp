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

#include "Math/AABB.hpp"
#include "Math/Ray.hpp"
#include "World/Components/MeshRenderer.hpp"
#include "World/Entity.hpp"
#include "World/World.hpp"

#include <optional>

namespace world
{

// ****************************************************************************
//! \brief One MeshRenderer the geometric raycast hit.
//!
//! Distance is along the ray, in world units, because the Ray direction is
//! unit length. The point is \c ray.pointAt(distance).
// ****************************************************************************
struct RayHit
{
    Entity entity{};
    float distance = 0.0f;
    Vector3f point{ 0.0f, 0.0f, 0.0f };
};

// ****************************************************************************
//! \brief Closest MeshRenderer whose supplied world AABB the ray enters.
//!
//! The World does not own mesh bounds — those live on the Asset. The caller
//! provides them, typically by transforming \c MeshAsset::local_bounds with
//! the entity's world matrix. That keeps this function free of \c assets::
//! and of \c gpu::, so a headless test can pick with synthetic boxes.
//!
//! Disabled entities and entities that are not alive are skipped. An empty
//! box is a miss.
//!
//! \tparam BoundsFn callable \c AABB(Entity, MeshRenderer const&).
// ****************************************************************************
template <typename BoundsFn>
[[nodiscard]] std::optional<RayHit> raycast(World const& p_world,
                                            Ray const& p_ray,
                                            BoundsFn&& p_world_bounds)
{
    auto const& store = p_world.components<MeshRenderer>();
    auto const renderers = store.components();
    auto const entities = store.entities();

    std::optional<RayHit> best;
    for (std::size_t i = 0u; i < renderers.size(); ++i)
    {
        const Entity entity = entities[i];
        if (!p_world.alive(entity) || !p_world.enabledInHierarchy(entity))
        {
            continue;
        }
        const AABB box = p_world_bounds(entity, renderers[i]);
        const std::optional<float> t = intersect(p_ray, box);
        if (!t.has_value())
        {
            continue;
        }
        if (best.has_value() && (*t >= best->distance))
        {
            continue;
        }
        RayHit hit;
        hit.entity = entity;
        hit.distance = *t;
        hit.point = p_ray.pointAt(*t);
        best = hit;
    }
    return best;
}

} // namespace world
