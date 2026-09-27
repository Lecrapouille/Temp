//=============================================================================
// Compages: A C++20 GPU, rendering and simulation library.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//=============================================================================

#include "Compages/Physics/PhysicsWorld.hpp"

#include "Compages/Math/Transformation.hpp"
#include "Compages/World/Components/BoxCollider.hpp"
#include "Compages/World/Components/CapsuleCollider.hpp"
#include "Compages/World/Components/RigidBody.hpp"
#include "Compages/World/Components/SphereCollider.hpp"
#include "Compages/World/EventQueue.hpp"
#include "Compages/World/World.hpp"

#include <reactphysics3d/reactphysics3d.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace physics
{

namespace rp3d = reactphysics3d;

namespace
{

[[nodiscard]] rp3d::Vector3 toRp3d(Vector3f const& p_value)
{
    return rp3d::Vector3(p_value.x, p_value.y, p_value.z);
}

[[nodiscard]] Vector3f fromRp3d(rp3d::Vector3 const& p_value)
{
    return Vector3f(p_value.x, p_value.y, p_value.z);
}

[[nodiscard]] rp3d::Quaternion toRp3d(Quatf const& p_value)
{
    return rp3d::Quaternion(
        p_value[1], p_value[2], p_value[3], p_value[0]);
}

[[nodiscard]] Quatf fromRp3d(rp3d::Quaternion const& p_value)
{
    return Quatf(p_value.w, p_value.x, p_value.y, p_value.z);
}

[[nodiscard]] Vector3f scaleOf(Matrix44f const& p_matrix)
{
    auto length = [&](std::size_t p_row) {
        const float x = p_matrix(p_row, 0u);
        const float y = p_matrix(p_row, 1u);
        const float z = p_matrix(p_row, 2u);
        return std::sqrt((x * x) + (y * y) + (z * z));
    };
    return Vector3f(length(0u), length(1u), length(2u));
}

[[nodiscard]] Quatf rotationOf(Matrix44f const& p_matrix)
{
    Matrix44f rotation(p_matrix);
    const Vector3f scale = scaleOf(p_matrix);
    for (std::size_t row = 0u; row < 3u; ++row)
    {
        const float divisor = std::max(scale[row], 1.0e-6f);
        for (std::size_t column = 0u; column < 3u; ++column)
        {
            rotation(row, column) /= divisor;
        }
    }
    rotation(3u, 0u) = 0.0f;
    rotation(3u, 1u) = 0.0f;
    rotation(3u, 2u) = 0.0f;
    return Quatf::fromMatrix(rotation);
}

[[nodiscard]] rp3d::Transform physicsTransform(world::World const& p_world,
                                               world::Entity p_entity)
{
    Matrix44f const& matrix = p_world.worldMatrix(p_entity);
    return rp3d::Transform(
        rp3d::Vector3(matrix(3u, 0u), matrix(3u, 1u), matrix(3u, 2u)),
        toRp3d(rotationOf(matrix)));
}

void writeWorldPose(world::World& p_world,
                    world::Entity p_entity,
                    rp3d::Transform const& p_transform)
{
    const Vector3f position = fromRp3d(p_transform.getPosition());
    const Quatf rotation = fromRp3d(p_transform.getOrientation());
    world::Entity const parent = p_world.parent(p_entity);

    world::LocalTransformView local = p_world.transform(p_entity);
    if (!parent.valid())
    {
        local.position = position;
        local.rotation = rotation;
        return;
    }

    const Matrix44f world_pose =
        world::composeLocalMatrix(
            position, rotation, Vector3f(1.0f, 1.0f, 1.0f));
    const Matrix44f local_pose =
        world_pose * matrix::inverse(p_world.worldMatrix(parent));
    local.position = Vector3f(local_pose(3u, 0u),
                              local_pose(3u, 1u),
                              local_pose(3u, 2u));
    local.rotation = rotationOf(local_pose);
}

[[nodiscard]] rp3d::BodyType bodyType(world::BodyType p_type)
{
    switch (p_type)
    {
        case world::BodyType::Static:
            return rp3d::BodyType::STATIC;
        case world::BodyType::Kinematic:
            return rp3d::BodyType::KINEMATIC;
        case world::BodyType::Dynamic:
            return rp3d::BodyType::DYNAMIC;
    }
    return rp3d::BodyType::STATIC;
}

enum class ShapeKind : std::uint8_t
{
    Box,
    Sphere,
    Capsule,
};

struct ShapeDesc
{
    ShapeKind kind = ShapeKind::Box;
    Vector3f extent{ 0.5f, 0.5f, 0.5f };
    float radius = 0.5f;
    float height = 1.0f;
    bool trigger = false;

    [[nodiscard]] bool operator==(ShapeDesc const& p_other) const
    {
        return (kind == p_other.kind) &&
               (extent.x == p_other.extent.x) &&
               (extent.y == p_other.extent.y) &&
               (extent.z == p_other.extent.z) &&
               (radius == p_other.radius) &&
               (height == p_other.height) &&
               (trigger == p_other.trigger);
    }
};

struct BodyRuntime
{
    world::Entity entity;
    rp3d::RigidBody* body = nullptr;
    rp3d::Collider* collider = nullptr;
    rp3d::CollisionShape* shape = nullptr;
    ShapeDesc desc;
};

} // namespace

struct PhysicsWorld::Impl
{
    struct Listener final : rp3d::EventListener
    {
        world::EventQueue* events = nullptr;

        void onContact(
            rp3d::CollisionCallback::CallbackData const& p_data) override
        {
            if (events == nullptr)
            {
                return;
            }
            for (rp3d::uint32 i = 0u;
                 i < p_data.getNbContactPairs();
                 ++i)
            {
                auto const pair = p_data.getContactPair(i);
                if (pair.getEventType() ==
                    rp3d::CollisionCallback::ContactPair::EventType::ContactExit)
                {
                    continue;
                }
                auto const* first =
                    static_cast<BodyRuntime*>(pair.getBody1()->getUserData());
                auto const* second =
                    static_cast<BodyRuntime*>(pair.getBody2()->getUserData());
                if ((first != nullptr) && (second != nullptr))
                {
                    events->push(world::Event{
                        world::EventKind::Collision,
                        first->entity,
                        second->entity });
                }
            }
        }

        void onTrigger(
            rp3d::OverlapCallback::CallbackData const& p_data) override
        {
            if (events == nullptr)
            {
                return;
            }
            for (rp3d::uint32 i = 0u;
                 i < p_data.getNbOverlappingPairs();
                 ++i)
            {
                auto const pair = p_data.getOverlappingPair(i);
                if (pair.getEventType() ==
                    rp3d::OverlapCallback::OverlapPair::EventType::OverlapExit)
                {
                    continue;
                }
                auto const* first =
                    static_cast<BodyRuntime*>(pair.getBody1()->getUserData());
                auto const* second =
                    static_cast<BodyRuntime*>(pair.getBody2()->getUserData());
                if ((first != nullptr) && (second != nullptr))
                {
                    events->push(world::Event{
                        world::EventKind::Trigger,
                        first->entity,
                        second->entity });
                }
            }
        }
    };

    rp3d::PhysicsCommon common;
    rp3d::PhysicsWorld* world = nullptr;
    std::unordered_map<std::uint32_t, std::unique_ptr<BodyRuntime>> bodies;
    Listener listener;
    float accumulator = 0.0f;
    static constexpr float FIXED_STEP = 1.0f / 60.0f;

    Impl()
    {
        world = common.createPhysicsWorld();
        world->setEventListener(&listener);
    }

    ~Impl()
    {
        clear();
        common.destroyPhysicsWorld(world);
    }

    void destroyShape(BodyRuntime& p_runtime)
    {
        switch (p_runtime.desc.kind)
        {
            case ShapeKind::Box:
                common.destroyBoxShape(
                    static_cast<rp3d::BoxShape*>(p_runtime.shape));
                break;
            case ShapeKind::Sphere:
                common.destroySphereShape(
                    static_cast<rp3d::SphereShape*>(p_runtime.shape));
                break;
            case ShapeKind::Capsule:
                common.destroyCapsuleShape(
                    static_cast<rp3d::CapsuleShape*>(p_runtime.shape));
                break;
        }
    }

    void destroy(std::uint32_t p_key)
    {
        auto const found = bodies.find(p_key);
        if (found == bodies.end())
        {
            return;
        }
        BodyRuntime& runtime = *found->second;
        world->destroyRigidBody(runtime.body);
        destroyShape(runtime);
        bodies.erase(found);
    }

    void clear()
    {
        while (!bodies.empty())
        {
            destroy(bodies.begin()->first);
        }
    }

    [[nodiscard]] rp3d::CollisionShape* makeShape(ShapeDesc const& p_desc)
    {
        switch (p_desc.kind)
        {
            case ShapeKind::Box:
                return common.createBoxShape(toRp3d(p_desc.extent));
            case ShapeKind::Sphere:
                return common.createSphereShape(p_desc.radius);
            case ShapeKind::Capsule:
                return common.createCapsuleShape(
                    p_desc.radius, p_desc.height);
        }
        return nullptr;
    }

    void create(world::World& p_world,
                world::Entity p_entity,
                ShapeDesc const& p_desc)
    {
        auto runtime = std::make_unique<BodyRuntime>();
        runtime->entity = p_entity;
        runtime->desc = p_desc;
        runtime->shape = makeShape(p_desc);
        runtime->body =
            world->createRigidBody(physicsTransform(p_world, p_entity));
        runtime->collider = runtime->body->addCollider(
            runtime->shape, rp3d::Transform::identity());
        runtime->collider->setIsTrigger(p_desc.trigger);
        runtime->body->setUserData(runtime.get());
        bodies.emplace(p_entity.bits(), std::move(runtime));
    }

    void ensure(world::World& p_world,
                world::Entity p_entity,
                ShapeDesc const& p_desc)
    {
        auto const found = bodies.find(p_entity.bits());
        if ((found != bodies.end()) && (found->second->desc == p_desc))
        {
            return;
        }
        destroy(p_entity.bits());
        create(p_world, p_entity, p_desc);
    }
};

PhysicsWorld::PhysicsWorld()
    : m_impl(std::make_unique<Impl>())
{
}

PhysicsWorld::~PhysicsWorld() = default;
PhysicsWorld::PhysicsWorld(PhysicsWorld&&) noexcept = default;
PhysicsWorld& PhysicsWorld::operator=(PhysicsWorld&&) noexcept = default;

std::optional<RaycastHit>
PhysicsWorld::raycast(Vector3f p_origin,
                      Vector3f p_direction,
                      float p_max_distance) const
{
    auto finite = [](Vector3f const& p_value) {
        return std::isfinite(p_value.x) &&
               std::isfinite(p_value.y) &&
               std::isfinite(p_value.z);
    };
    if (!finite(p_origin) || !finite(p_direction) ||
        !std::isfinite(p_max_distance) || (p_max_distance <= 0.0f))
    {
        return std::nullopt;
    }

    const float length_squared =
        (p_direction.x * p_direction.x) +
        (p_direction.y * p_direction.y) +
        (p_direction.z * p_direction.z);
    if (!std::isfinite(length_squared) || (length_squared <= 0.0f))
    {
        return std::nullopt;
    }
    const float inverse_length = 1.0f / std::sqrt(length_squared);
    const Vector3f end =
        p_origin + (p_direction * inverse_length * p_max_distance);
    if (!finite(end))
    {
        return std::nullopt;
    }

    struct ClosestHit final : rp3d::RaycastCallback
    {
        std::optional<RaycastHit> hit;
        float max_distance = 0.0f;

        rp3d::decimal notifyRaycastHit(
            rp3d::RaycastInfo const& p_info) override
        {
            auto const* runtime =
                static_cast<BodyRuntime const*>(p_info.body->getUserData());
            if (runtime == nullptr)
            {
                return rp3d::decimal(-1.0);
            }
            RaycastHit candidate;
            candidate.entity = runtime->entity;
            candidate.point = fromRp3d(p_info.worldPoint);
            candidate.normal = fromRp3d(p_info.worldNormal);
            candidate.fraction = static_cast<float>(p_info.hitFraction);
            candidate.distance = candidate.fraction * max_distance;
            if (!hit || (candidate.fraction < hit->fraction))
            {
                hit = candidate;
            }
            return p_info.hitFraction;
        }
    };

    ClosestHit callback;
    callback.max_distance = p_max_distance;
    m_impl->world->raycast(
        rp3d::Ray(toRp3d(p_origin), toRp3d(end)), &callback);
    return callback.hit;
}

void PhysicsWorld::step(world::World& p_world,
                        float p_dt,
                        world::EventQueue* p_events)
{
    p_world.update();
    m_impl->world->setGravity(toRp3d(gravity));

    std::unordered_set<std::uint32_t> candidates;
    auto registerShape = [&](world::Entity p_entity, ShapeDesc p_desc) {
        if (!candidates.insert(p_entity.bits()).second)
        {
            return;
        }
        m_impl->ensure(p_world, p_entity, p_desc);
    };

    p_world.each<world::BoxCollider>(
        [&](world::Entity p_entity, world::BoxCollider const& p_box) {
        const Vector3f scale = scaleOf(p_world.worldMatrix(p_entity));
        registerShape(
            p_entity,
            ShapeDesc{ ShapeKind::Box,
                       Vector3f(std::abs(p_box.half_extent.x * scale.x),
                                std::abs(p_box.half_extent.y * scale.y),
                                std::abs(p_box.half_extent.z * scale.z)),
                       0.0f,
                       0.0f,
                       p_box.trigger });
    });
    p_world.each<world::SphereCollider>(
        [&](world::Entity p_entity, world::SphereCollider const& p_sphere) {
        const Vector3f scale = scaleOf(p_world.worldMatrix(p_entity));
        const float largest = std::max({ std::abs(scale.x),
                                         std::abs(scale.y),
                                         std::abs(scale.z) });
        ShapeDesc desc;
        desc.kind = ShapeKind::Sphere;
        desc.radius = std::abs(p_sphere.radius * largest);
        desc.trigger = p_sphere.trigger;
        registerShape(p_entity, desc);
    });
    p_world.each<world::CapsuleCollider>(
        [&](world::Entity p_entity, world::CapsuleCollider const& p_capsule) {
        const Vector3f scale = scaleOf(p_world.worldMatrix(p_entity));
        ShapeDesc desc;
        desc.kind = ShapeKind::Capsule;
        desc.radius = std::abs(
            p_capsule.radius * std::max(scale.x, scale.z));
        desc.height = std::abs(p_capsule.height * scale.y);
        desc.trigger = p_capsule.trigger;
        registerShape(p_entity, desc);
    });

    for (auto it = m_impl->bodies.begin(); it != m_impl->bodies.end();)
    {
        if (!p_world.alive(it->second->entity) ||
            !candidates.contains(it->first))
        {
            const std::uint32_t key = it->first;
            ++it;
            m_impl->destroy(key);
        }
        else
        {
            ++it;
        }
    }

    for (auto& [key, runtime_ptr] : m_impl->bodies)
    {
        (void)key;
        BodyRuntime& runtime = *runtime_ptr;
        world::RigidBody* component =
            p_world.tryGet<world::RigidBody>(runtime.entity);
        const world::BodyType type =
            (component != nullptr) ? component->type
                                   : world::BodyType::Static;
        runtime.body->setType(bodyType(type));

        if (component != nullptr)
        {
            runtime.body->setMass(std::max(component->mass, 0.001f));
            runtime.body->setLinearDamping(component->linear_damping);
            runtime.body->setAngularDamping(component->angular_damping);
            runtime.body->enableGravity(component->gravity_enabled);
            runtime.collider->getMaterial().setFrictionCoefficient(
                component->friction);
            runtime.collider->getMaterial().setBounciness(
                component->restitution);
            if (type == world::BodyType::Dynamic)
            {
                runtime.body->setLinearVelocity(toRp3d(component->velocity));
                runtime.body->setAngularVelocity(
                    toRp3d(component->angular_velocity));
            }
        }

        if (type != world::BodyType::Dynamic)
        {
            runtime.body->setTransform(
                physicsTransform(p_world, runtime.entity));
        }
    }

    m_impl->listener.events = p_events;
    m_impl->accumulator += std::clamp(p_dt, 0.0f, 0.25f);
    while (m_impl->accumulator >= Impl::FIXED_STEP)
    {
        m_impl->world->update(Impl::FIXED_STEP);
        m_impl->accumulator -= Impl::FIXED_STEP;
    }
    m_impl->listener.events = nullptr;

    for (auto& [key, runtime_ptr] : m_impl->bodies)
    {
        (void)key;
        BodyRuntime& runtime = *runtime_ptr;
        world::RigidBody* component =
            p_world.tryGet<world::RigidBody>(runtime.entity);
        if ((component == nullptr) ||
            (component->type != world::BodyType::Dynamic))
        {
            continue;
        }
        component->velocity =
            fromRp3d(runtime.body->getLinearVelocity());
        component->angular_velocity =
            fromRp3d(runtime.body->getAngularVelocity());
        writeWorldPose(
            p_world, runtime.entity, runtime.body->getTransform());
    }

    p_world.update();
}

} // namespace physics
