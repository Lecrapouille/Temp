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

#include "Physics/PhysicsWorld.hpp"

#include "Math/AABB.hpp"
#include "World/Components/BoxCollider.hpp"
#include "World/Components/RigidBody.hpp"
#include "World/EventQueue.hpp"
#include "World/World.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace physics
{

namespace
{

struct ColliderEntry
{
    world::Entity entity;
    AABB bounds;
    world::BodyType type;
};

[[nodiscard]] AABB worldColliderBounds(world::World const& p_world,
                                       world::Entity p_entity,
                                       world::BoxCollider const& p_collider)
{
    const world::LocalTransform& local = p_world.transform(p_entity);
    const Vector3f extent(
        p_collider.half_extent.x * std::abs(local.scale.x),
        p_collider.half_extent.y * std::abs(local.scale.y),
        p_collider.half_extent.z * std::abs(local.scale.z));
    return AABB::fromCenterExtent(local.position, extent);
}

[[nodiscard]] bool isDynamic(world::BodyType p_type)
{
    return p_type == world::BodyType::Dynamic;
}

// After the axis flip below, `normal` points from A toward B. A must move
// along -normal and B along +normal, otherwise two dynamics pull together
// and nest. Tiny leftover overlap is left alone so a stack does not jitter.
constexpr float kPenetrationSlop = 0.005f;
constexpr int kSolverIterations = 5;

void refreshBounds(world::World const& p_world, ColliderEntry& p_entry)
{
    world::BoxCollider const* box =
        p_world.tryGet<world::BoxCollider>(p_entry.entity);
    if (box != nullptr)
    {
        p_entry.bounds = worldColliderBounds(p_world, p_entry.entity, *box);
    }
}

void resolvePair(world::World& p_world,
                 ColliderEntry& p_a,
                 ColliderEntry& p_b,
                 world::EventQueue* p_events)
{
    if (!p_a.bounds.intersects(p_b.bounds))
    {
        return;
    }

    const bool a_dynamic = isDynamic(p_a.type);
    const bool b_dynamic = isDynamic(p_b.type);
    if (!a_dynamic && !b_dynamic)
    {
        return;
    }

    const Vector3f overlap_min(
        std::max(p_a.bounds.min.x, p_b.bounds.min.x),
        std::max(p_a.bounds.min.y, p_b.bounds.min.y),
        std::max(p_a.bounds.min.z, p_b.bounds.min.z));
    const Vector3f overlap_max(
        std::min(p_a.bounds.max.x, p_b.bounds.max.x),
        std::min(p_a.bounds.max.y, p_b.bounds.max.y),
        std::min(p_a.bounds.max.z, p_b.bounds.max.z));
    const Vector3f penetration = overlap_max - overlap_min;

    // Side-by-side bodies have a large Y overlap (same height) and a smaller
    // X/Z overlap. Picking the global minimum then launches them and they
    // hover. If the centres are more separated horizontally than vertically,
    // this is a side hit: only X/Z may separate them.
    const Vector3f center_a = p_a.bounds.center();
    const Vector3f center_b = p_b.bounds.center();
    const float dx = std::abs(center_a.x - center_b.x);
    const float dy = std::abs(center_a.y - center_b.y);
    const float dz = std::abs(center_a.z - center_b.z);
    const bool side_hit = std::max(dx, dz) >= dy;

    Vector3f normal{ 1.0f, 0.0f, 0.0f };
    float depth = penetration.x;
    if (penetration.z < depth)
    {
        depth = penetration.z;
        normal = Vector3f(0.0f, 0.0f, 1.0f);
    }
    if (!side_hit && (penetration.y < depth))
    {
        depth = penetration.y;
        normal = Vector3f(0.0f, 1.0f, 0.0f);
    }

    if (depth <= kPenetrationSlop)
    {
        return;
    }

    if (p_events != nullptr)
    {
        p_events->push(world::Event{ world::EventKind::Collision,
                                     p_a.entity,
                                     p_b.entity });
    }

    if (vector::dot(center_a - center_b, normal) > 0.0f)
    {
        normal = -normal;
    }

    const float correction = depth - kPenetrationSlop;

    auto pushDynamic = [&](ColliderEntry& p_entry, float p_along_normal) {
        world::RigidBody* body = p_world.tryGet<world::RigidBody>(p_entry.entity);
        if ((body == nullptr) || !isDynamic(body->type))
        {
            return;
        }
        world::LocalTransform& tr = p_world.transform(p_entry.entity);
        tr.position = tr.position + (normal * (correction * p_along_normal));
        const float vn = vector::dot(body->velocity, normal);
        if (vn * p_along_normal < 0.0f)
        {
            body->velocity = body->velocity - (normal * vn);
        }
        refreshBounds(p_world, p_entry);
    };

    if (a_dynamic && b_dynamic)
    {
        pushDynamic(p_a, -0.5f);
        pushDynamic(p_b, 0.5f);
    }
    else if (a_dynamic)
    {
        pushDynamic(p_a, -1.0f);
    }
    else
    {
        pushDynamic(p_b, 1.0f);
    }
}

} // namespace

//------------------------------------------------------------------------------
void PhysicsWorld::step(world::World& p_world,
                        float p_dt,
                        world::EventQueue* p_events)
{
    world::ComponentStore<world::RigidBody>& bodies =
        p_world.components<world::RigidBody>();
    for (std::size_t i = 0u; i < bodies.size(); ++i)
    {
        world::Entity entity = bodies.entities()[i];
        world::RigidBody& body = bodies.components()[i];
        if (body.type != world::BodyType::Dynamic)
        {
            continue;
        }
        body.velocity = body.velocity + (gravity * p_dt);
        body.velocity = body.velocity * std::max(0.0f, 1.0f - (1.2f * p_dt));
        p_world.transform(entity).position =
            p_world.transform(entity).position + (body.velocity * p_dt);
    }

    std::vector<ColliderEntry> colliders;
    world::ComponentStore<world::BoxCollider> const& boxes =
        p_world.components<world::BoxCollider>();
    colliders.reserve(boxes.size());
    for (std::size_t i = 0u; i < boxes.size(); ++i)
    {
        world::Entity entity = boxes.entities()[i];
        world::BodyType type = world::BodyType::Static;
        if (world::RigidBody const* body = p_world.tryGet<world::RigidBody>(entity))
        {
            type = body->type;
        }
        colliders.push_back(
            ColliderEntry{ entity,
                           worldColliderBounds(p_world, entity, boxes.components()[i]),
                           type });
    }

    for (int iteration = 0; iteration < kSolverIterations; ++iteration)
    {
        world::EventQueue* events =
            (iteration == 0) ? p_events : nullptr;
        for (std::size_t i = 0u; i < colliders.size(); ++i)
        {
            for (std::size_t j = i + 1u; j < colliders.size(); ++j)
            {
                resolvePair(p_world, colliders[i], colliders[j], events);
            }
        }
    }

    p_world.update();
}

} // namespace physics
